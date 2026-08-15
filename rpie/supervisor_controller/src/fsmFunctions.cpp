//AUTHOR: Emiliano Perez Pellicer 
//DATE: 03/07/2026
//COMMENTS: This is a finite state machine & Sabbath Mode 
//VERSION: 1.0 
#include "../include/pcanFunctions.h"
#include "../include/mainFunctions.h"
#include "../include/databaseFunctions.h"
#include "../include/fsmFuntions.h"
#include "../include/audioFunctions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// Internal Helpers 

static volatile sig_atomic_t g_fsmStop = 0; 

static void fsmSigintHandler(int sig){
    (void)sig;
    g_fsmStop  = 1; //let the main loop exit cleanly and close CAN handler
}

//Monotonic milliseconds. time_t/difftime() only counts whole seconds, which is too
//coarse to poll the website faster than once a second - and made the real interval
//drift between 1s and 2s. CLOCK_MONOTONIC also cannot jump if the clock is adjusted.
static long long fsmNowMs(void){
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

//Throttles a debug line for a phase still in progress: true on the FIRST tick
//(*last == 0), then once every periodSec, so the 5Hz loop cannot repeat itself.
//Callers MUST zero *last when the phase ends so the next entry prints at once.
static bool fsmShouldPrint(time_t *last, int periodSec){
    time_t now = time(NULL);

    if(*last != 0 && difftime(now, *last) < periodSec){
        return false;
    }

    *last = now;
    return true;
}

const char* fsmStateName(ElevatorSate s){
    switch (s){
    case STATE_FLOOR1:
        return "ON FLOOR 1"; 
        break;

    case STATE_FLOOR2:
        return "ON FLOOR 2"; 
        break; 

    case STATE_FLOOR3:
        return "ON FLOOR 3"; 
        break; 

    case STATE_MOVING:
        return "MOVING";
        break; 

    default:
        return "UNKNOWN";
        break;
    }
}

//Change the mode name 
const char* modeName(ElevatorMode m){
    switch (m){

    case MODE_SABBATH:
        return "sabbath"; 
        break;

    case MODE_MAINTEANCE:
        return "maintenance"; 
    
    default:
        return "elevator"; 
        break;
    }
}


//Initial Condition 
void fsmInit(ElevatorFSM *fsm){
    fsm->state = STATE_FLOOR1; 
    //Add the queues 
    fsm->carQueue = std::queue<int>();
    fsm->floorQueue = std::queue<int>();
    fsm->websiteQueue = std::queue<int>();
    
    //Elevartor 
    fsm->doorClosed =0; //Door Starts Open
    fsm->previousFloor = 1; 
    fsm->targetFloor = 1;

    //Website 
    fsm->lastWebsitePollMs = fsmNowMs();
    fsm->modeId = MODE_ELEVATOR;
    fsm->pendingMode = MODE_ELEVATOR; 
    fsm->mode = modeName(MODE_ELEVATOR); 

    //Timers 
    fsm->doorTimeStart = time(NULL); 
    fsm->movingTimeStart = 0; 
    fsm->collectTimerStart = 0; //window for listening (elevator door is open right now )
    fsm->preMoveTimerStart = 0;
    
    //Target 
    fsm->lockedTarget =0;
    
    //Sentinels - guarantee the first fsmPublishPosition() always writes a row
    fsm->pubFloor = -1;
    fsm->pubLast = -1;
    fsm->pubMoving = -1;
    fsm->pubClosed = -1;
    fsm->pubMode = NULL;
}

//Queue Helpers 
static bool queueContainsFloor(std::queue<int> q, int floor){
    while(!q.empty()){
        //Check if the current floor is a the head of the queue 
        if(q.front() == floor){
            return true; 
        }
        //If so, pop it to add the next one 
        q.pop();
    }
    return false; 
}

static void purgeFloorFromQueue(std::queue<int> &q, int floor){
    std::queue<int> kept; 
    while(!q.empty()){
        //if the front of the queue is a floor push it out 
        int f = q.front(); 
        q.pop(); 
        if(f != floor){
            kept.push(f); 
        }
    }  

    q = kept; 
}

// A floor arrived or picked as the next mocing target 
// satisfisies that floor request in all three queues at once 
static void fsmConsumeRequestsForFloor(ElevatorFSM *fsm, int floor){
    purgeFloorFromQueue(fsm->carQueue, floor);
    purgeFloorFromQueue(fsm->floorQueue, floor);
    purgeFloorFromQueue(fsm->websiteQueue, floor);
}

//Priotity queue 
static int fsmPeekPriotity(ElevatorFSM *fsm, int currentFloor){
    std::queue<int> tmp; 

    tmp = fsm->carQueue; 
    while(!tmp.empty()){
        //Give a value to the current foor
        int f = tmp.front(); 
        //pop that floor 
        tmp.pop(); 
        if(f != currentFloor){
            return f; 
        }
    }

    tmp = fsm->floorQueue; 
    while(!tmp.empty()){
        //Give a value to the current foor
        int f = tmp.front(); 
        //pop that floor 
        tmp.pop(); 
        if(f != currentFloor){
            return f; 
        }
    }

    tmp = fsm->websiteQueue; 
    while(!tmp.empty()){
        //Give a value to the current foor
        int f = tmp.front(); 
        //pop that floor 
        tmp.pop(); 
        if(f != currentFloor){
            return f; 
        }
    }

    return 0;

}

//Changes the door state AND announces it - the CC drives its door LEDs off this frame.
//Every writer of doorClosed goes through here EXCEPT telemetry coming back from the CC.
static void fsmSetDoor(ElevatorFSM *fsm, int doorState){
    fsm->doorClosed = doorState;
    fsm->doorTimeStart = time(NULL);
    fsm->collectTimerStart = 0;

    pcanFsmTx(ID_SC_TO_EC, doorState);
}

//Polls the DB for a website requedted floor at most once every Macro preivously define
//Called once per supervisorRun() iteration in EVERY mode, including Sabbath - that is
//the only way a mode change can reach the FSM and get the car back out of Sabbath.
//The per-command guard below is what keeps Sabbath ignoring everything else.
static void fsmPollWebsite(ElevatorFSM *fsm){
    long long now = fsmNowMs();

    if(now - fsm->lastWebsitePollMs < WEBSITE_POLL_MS){
        return;
    }
    fsm->lastWebsitePollMs = now;

    WebCommand cmds[8]; 
    int n = db_getWebsiteCommands(cmds, 8); 
    if(n <= 0){
        return; //Notable to connect 
    }

    for(int i = 0; i < n; i++){
        //Sabbath ignores every REQUEST - car, floor and door alike; only a mode change
        //is honoured, otherwise there is no way back out from the website. The command
        //is still consumed from the cursor and already logged by the website.
        if(fsm->modeId == MODE_SABBATH && cmds[i].kind != WEB_CMD_MODE){
            continue;
        }

        switch(cmds[i].kind){
            case WEB_CMD_FLOOR_CAR:
                if(!queueContainsFloor(fsm->carQueue, cmds[i].floor)){
                    fsm->carQueue.push(cmds[i].floor); 
                    printf("[FSM] Website Car Request queued (Priority 1): FLOOR %d\n", cmds[i].floor);
                }
            break; 
            
            case WEB_CMD_FLOOR_REQ:
                if(!queueContainsFloor(fsm->floorQueue, cmds[i].floor)){
                    fsm->floorQueue.push(cmds[i].floor);
                    printf("[FSM] Website Floor Request queued (Priority 2): FLOOR %d\n", cmds[i].floor);
                }
            break; 

            case WEB_CMD_MODE:
                if(cmds[i].mode == WEB_MODE_SABBATH){
                    fsm->pendingMode = MODE_SABBATH; 
                }else if(cmds[i].mode == WEB_MODE_MAINTEANCE){
                    fsm->pendingMode = MODE_MAINTEANCE; 
                }else{
                    fsm->pendingMode = MODE_ELEVATOR; 
                }
                printf("[FSM] Website requested MODE: %s\n", modeName(fsm->pendingMode));
            break;

            case WEB_CMD_DOOR:
                //Updating doorClosed is what changes behaviour: fsmStepInner() refuses
                //to depart while the door is open, and re-opens the listening window
                //once it closes.
                if(fsm->state == STATE_MOVING){
                    //Never touch the door mid-travel
                    printf("[FSM] Website DOOR command IGNORED - car is MOVING to FLOOR %d\n", fsm->targetFloor);
                    break;
                }

                if(cmds[i].door == WEB_DOOR_OPEN){
                    //Drop anything already locked in so the car cannot pull away with
                    //a door the website just opened
                    fsm->lockedTarget = 0;
                    fsm->preMoveTimerStart = 0;
                }

                //Also zeroes collectTimerStart, which on CLOSE starts a fresh
                //REQUEST_COLLECTION_SEC window
                fsmSetDoor(fsm, cmds[i].door);

                printf("[FSM] Website DOOR %s at FLOOR %d\n",
                       (cmds[i].door == WEB_DOOR_OPEN) ? "OPEN" : "CLOSE",
                       (int)fsm->state + 1);
            break;

            default:
                break;
        }
    }
}


//Works out the (current, last) floor pair the website tables expect.
//While MOVING the "current" floor is the DESTINATION - is_moving = 1 is what tells
//the website the car is still in transit. Swap targetFloor for previousFloor here if
//the indicator should instead hold the departure floor mid-travel.
static void fsmFloorPair(const ElevatorFSM *fsm, int *outCurrent, int *outLast){
    if(fsm->state == STATE_MOVING){
        *outCurrent = fsm->targetFloor;
    }else{
        *outCurrent = (int)fsm->state + 1; //STATE_FLOOR1(0) -> 1
    }

    *outLast = fsm->previousFloor;
}


//Publishes the elevator's state - floor, motion, door and the running MODE - to the
//elevator_position table the website reads.
//Called once per FSM step, so it only inserts when something actually changed;
//otherwise the 5 Hz loop would add hundreds of identical rows a minute.
static void fsmPublishPosition(ElevatorFSM *fsm){
    int currentFloor;
    int lastFloor;

    fsmFloorPair(fsm, &currentFloor, &lastFloor);

    int isMoving = (fsm->state == STATE_MOVING) ? 1 : 0;
    //doorClosed holds DOOR_CLOSE/DOOR_OPEN, never a 0/1 flag - compare the macro.
    //fsmInit() leaves it at literal 0, which correctly reads as "not closed".
    int isClosed = (fsm->doorClosed == DOOR_CLOSE) ? 1 : 0;

    if(currentFloor == fsm->pubFloor &&
       lastFloor == fsm->pubLast &&
       isMoving == fsm->pubMoving &&
       isClosed == fsm->pubClosed &&
       fsm->mode == fsm->pubMode){
        return; //Nothing changed since the last row
    }

    if(db_insertElevatorPosition(currentFloor, lastFloor, isMoving, isClosed, fsm->mode) != 0){
        return; //Insert failed - leave the snapshot alone so we retry next step
    }

    fsm->pubFloor = currentFloor;
    fsm->pubLast = lastFloor;
    fsm->pubMoving = isMoving;
    fsm->pubClosed = isClosed;
    fsm->pubMode = fsm->mode;
}


//Records one CAN frame sensed on the bus into can_transaction.
//Called from every run loop BEFORE the mode-specific handler: Sabbath drops every
//request and Maintenance drops every hardware button, but those frames were still
//sensed and have to show up in the log.
static void fsmLogRxMessage(const ElevatorFSM *fsm, int id, int data, int len){
    char message[256];
    int currentFloor;
    int lastFloor;

    fsmFloorPair(fsm, &currentFloor, &lastFloor);

    //Reuse the decoders the RX printouts already use. can_transaction has no length
    //column, so the DLC goes into the message text.
    snprintf(message, sizeof(message), "%s: %s (LEN %d)",
             decodeSenderName(id), decodeMsgType(id, data), len);

    db_logCanTransaction(id, data, message, currentFloor, lastFloor);
}


//Whenever the elevator arrives at a floor
//EC_TO_ALL confirmaton, or from the MoVING fall back timeput
//doorState is a parameter so Sabbath - which parks OPEN - does not arrive CLOSED and
//then reopen, which would put two contradictory frames on the bus and flicker the LEDs.
static void fsmArriveAt(ElevatorFSM *fsm, int floor, int doorState){
    fsm->state = (ElevatorSate)(floor - 1); //STATE FLOOR - 1
    fsm->lockedTarget = 0;
    fsm->preMoveTimerStart = 0;

    //Zeroes collectTimerStart too - no listening window until the door closes again
    fsmSetDoor(fsm, doorState);

    fsmConsumeRequestsForFloor(fsm, floor);
    printf("[FSM] Arrived at Floor %d - door %s(%ds Timer started)\n",
           floor, (doorState == DOOR_CLOSE) ? "CLOSED" : "OPEN", DOOR_OPEN_TIME_SEC);

    audioPlayFloor(floor); //Ding + floor announcement - returns immediately (own audio thread)
}

static void fsmArrive(ElevatorFSM *fsm, int floor){
    fsmArriveAt(fsm, floor, DOOR_CLOSE);
}

//Applies one incoming CAN message to the FSM's request flags / arrival detection 
static void fsmProcessMessage(ElevatorFSM *fsm, int id, int data){
    int floor; 

    switch (id){
    case ID_F1_TO_SC:
        floor = 1; 
        if(!queueContainsFloor(fsm->floorQueue, floor)){
            fsm->floorQueue.push(floor);
            printf("[FSM] Floor-call request detected (Priority 2): FLOOR 1\n");
        }
        break;
    
    case ID_F2_TO_SC:
        floor = 2;
        if(!queueContainsFloor(fsm->floorQueue, floor)){
           fsm->floorQueue.push(floor);
           printf("[FSM] Floor-call request detected (Priotity 2): FLOOR 2\n"); 
        }
        break;
    
    case ID_F3_TO_SC:
        floor = 3;
        if(!queueContainsFloor(fsm->floorQueue, floor)){
            fsm->floorQueue.push(floor);
            printf("[FSM] Floor-call request detected (Priority 2): FLOOR 3\n");
        } 
        break;
    
    case ID_CC_TO_SC:
        floor = FloorFromHex(data);
        if(floor == 0){
            //Indicate if te door is oopen or closed
            fsm->doorClosed = data; 

        }else{
            if(!queueContainsFloor(fsm->carQueue, floor)){
                fsm->carQueue.push(floor);
                printf("[FSM] Car Request Detected (Prioptity 1): FLOOR %d\n", floor); 
            }  
        }
        break;
    
    case ID_EC_TO_ALL:
        if(fsm->state == STATE_MOVING){
            floor = FloorFromHex(data);
            if(floor == fsm->targetFloor){
                fsmArrive(fsm, floor); 
            }
        }
        break; 

    default:
    //do nothing 
        break;
    }
}

//Runs one FSM evaluation stem (door timer, request shcedueling, moving fallback )
//Split from fsmStep() only so the position publish still happens on the several
//early returns below.
static void fsmStepInner(ElevatorFSM *fsm){
    static time_t lastCollectingPrint = 0;
    static time_t lastDoorOpenPrint = 0;
    static time_t lastPreMovePrint = 0;

    //The website poll moved to supervisorRun() so Sabbath - which never calls this
    //function - can see mode changes too.

    if(fsm->state != STATE_MOVING){
        //Register the current floor 
        int currentFloor = (int)fsm->state + 1; //State Floor1(0) -> 1

        //DOOR IS OPEN 
        if(fsm->doorClosed != DOOR_CLOSE){
            fsmConsumeRequestsForFloor(fsm, currentFloor); //Requesrts at this floor are now beind done 
            fsm->collectTimerStart =0; //re start listening window
            fsm->lockedTarget =0;

            //Closed-door phases are not running - zero them so they print on entry
            lastCollectingPrint = 0;
            lastPreMovePrint = 0;

            if(fsmShouldPrint(&lastDoorOpenPrint, FSM_DEBUG_PERIOD_SEC)){
                printf("[FSM] Door OPEN at Floor %d\n", currentFloor);
            }

        }else if(fsm->doorClosed == DOOR_CLOSE && fsm->lockedTarget != 0){
            lastDoorOpenPrint = 0; //Door phase is over

            double waited = difftime(time(NULL), fsm->preMoveTimerStart);
            if(waited < PRE_MOVE_DELAY_SEC){
                if(fsmShouldPrint(&lastPreMovePrint, FSM_DEBUG_PERIOD_SEC)){
                    printf("[FSM] DEBUG: Wasiting to move to FLOOR %d (%.0f/%d) elapsed\n", fsm->lockedTarget, waited, PRE_MOVE_DELAY_SEC);
                }
                return; //Still waiting
            }

            lastPreMovePrint = 0; //Wait is over - next one starts fresh

            int target = fsm->lockedTarget;
            fsm->lockedTarget = 0;
            fsm->previousFloor = currentFloor;
            fsm->targetFloor = target;
            fsm->state = STATE_MOVING;
            fsm->movingTimeStart = time(NULL);
 
            printf("[FSM] State: %s -> MOVING (Floor %d -> Floor %d)\n", fsmStateName((ElevatorSate)(currentFloor - 1)), currentFloor, target);
 
            //Send command to EC to move - use the persistent, non-blocking FSM handle (pcanTx() opens a
            //SECOND handle + blocks 1s via sleep(1), which fights with the hfsm handle pcanFsmRxPoll() uses)
            pcanFsmTx(ID_SC_TO_EC, HexFromFloor(target));
            return;

        }
        //DOOR IS CLOSED && LISTENING FOR REQUEST 
        if(fsm->doorClosed == DOOR_CLOSE && fsm->collectTimerStart == 0){
            fsm->collectTimerStart = time(NULL);
            lastCollectingPrint = 0;
            lastDoorOpenPrint = 0;
            printf("[FSM] Listening for requests for %d seconds before choosing Next Stop\n",REQUEST_COLLECTION_SEC);
            return;
        }

        //NOW COLLECTING REQUEST
        double collecting = difftime(time(NULL), fsm->collectTimerStart);
        if(collecting < REQUEST_COLLECTION_SEC){
            if(fsmShouldPrint(&lastCollectingPrint, FSM_DEBUG_PERIOD_SEC)){
                printf("[FSM] DEBUG: still collecting (%.0f/%d)\n", collecting, REQUEST_COLLECTION_SEC);
            }
            return; //Still on the collecting window - keep collecting
        }

        //LISTENING WINDOW IS OVER, PICK NEXT TARGET 
        if(fsm->doorClosed == DOOR_OPEN){
            return;
        }else{
            int target = fsmPeekPriotity(fsm, currentFloor); 
            if(target != 0 && target != currentFloor){
                fsmConsumeRequestsForFloor(fsm, target); //locked in now - remove from queues 
                fsm->lockedTarget = target;
                fsm->preMoveTimerStart = time(NULL);
                fsm->collectTimerStart =0; //reset for next stop
                lastCollectingPrint = 0; //Collection phase is over
                lastPreMovePrint = 0;    //Pre-move wait starts now - print its first tick

                //Print what got locked in - actual move happens after Phase B (see top of this function)
                printf("[FSM] Locked FLOOR %d - waiting %d seconds before moving\n", target, PRE_MOVE_DELAY_SEC); 
                
            }else{
                //Nothing then restarts timer 
                fsm->collectTimerStart = time(NULL); 
            }
        }
        

    } else {
        //We are MOVING 
        //Either wait for a real EC_TO_ALL arrival messafe (Handleld in fsmProcessMessage ) if not recieved use timer 
        double elapsed = difftime(time(NULL), fsm->movingTimeStart); 
        if (elapsed >= MOVING_FALLBACK_SEC){
            printf("[FSM] No EC Arrival confirmation (0x101) recieved - using fallback timeout\n");
            fsmArrive(fsm, fsm->targetFloor);
        }
    }
}
//Swicthes the running supervisor between modes without breaking the CAN or FSM Only when not Movinf 
static void fsmApplyMode(ElevatorFSM *fsm, ElevatorMode target){
    if(fsm->modeId == target){
        return; 
    }

    printf("\n[MODE] %s -> %s\n", modeName(fsm->modeId), modeName(target));

    //Swicth the states 
    switch (target){
    case MODE_SABBATH:
        // No request at all 
        fsm->carQueue = std::queue<int>();
        fsm->floorQueue = std::queue<int>();
        fsm->websiteQueue = std::queue<int>();
        fsmSetDoor(fsm, DOOR_OPEN);
        break;

    case MODE_MAINTEANCE:
        //Digital e-stop purge every Hardware request. Hold in Place DOOR OPEN 
        fsm->carQueue = std::queue<int>();
        fsm->floorQueue = std::queue<int>();
        fsmSetDoor(fsm, DOOR_OPEN);
        break;
    
    default: //MODE ELEVATOR 
        break;
    }

    fsm->lockedTarget = 0; 
    fsm->preMoveTimerStart = 0; 
    fsm->collectTimerStart = 0; 
    fsm->modeId = target;
    fsm->pendingMode = target;
    fsm->mode = modeName(target);  
}

//One FSM step, then mirror the resulting state to the website.
//Every transition (arrival, depart, door open/close) passes through here once per
//loop iteration, so this single call catches them all.
static void fsmStep(ElevatorFSM *fsm){
    fsmStepInner(fsm);
    fsmPublishPosition(fsm);
}

//Function to Get the Arrival in Sabbarth 
//Update the arrival 
static void sabbathArrive(ElevatorFSM *fsm, int floor){
    if(floor < 1 || floor > 3){
        floor = 1; //Never let a bad target corrupt the state 
    }

    //Arrive straight into OPEN so only one door frame reaches the CC
    fsmArriveAt(fsm, floor, DOOR_OPEN);
    printf("[SBTH] Arrived at Floor %d - door OPEN (%ds)\n", floor, DOOR_OPEN_TIME_SEC);
}

//Function for Sabbath mode counting on the EC for arrival confirmation 
static void sabbathProcessMessages(ElevatorFSM *fsm, int id, int data){
    if(id != ID_EC_TO_ALL || fsm->state != STATE_MOVING){
        return; //Ignore 
    }

    //Upodate the arrive status 
    if(FloorFromHex(data) == fsm->targetFloor){
        sabbathArrive(fsm, fsm->targetFloor);
    }
}

//Function to define Sabbath Targets 
static int sabbathMove(ElevatorFSM *fsm){
    
    //Find the Current Floor 
    int currentFloor = (int)fsm->state +1; 
    int previousFloor = fsm->previousFloor; 
    int nextfloor = 0; 

    switch (currentFloor)
    {
    case 1:
        if(previousFloor == 2){
            nextfloor = 2; 
        }else{
            nextfloor = 2;
        } 
        return nextfloor; 
        break;
    
    case 2:
        if(previousFloor == 1){
            nextfloor =  3; 
        }else{
            nextfloor =  1; 
        }
        return nextfloor; 
        break;
    
    case 3:
        if(previousFloor == 2){
            nextfloor = 2; 
        }else{
            nextfloor = 1;
        }
        return nextfloor; 
        break;
    
    default:
        nextfloor = 1; 
        return nextfloor; 
        break;
    }

    return nextfloor; 
}

///Stepping between Floors on Sabbath mode
//Split from sabbathStep() only so the position publish still happens on the several
//early returns below.
static void sabbathStepInner(ElevatorFSM *fsm){
    static time_t lastPreMovePrint = 0;

   if(fsm->state != STATE_MOVING){
    //Register current Floor 
    int currentFloor = (int)fsm->state +1; 

    //Door is OPEN 
    if(fsm->doorClosed != DOOR_CLOSE){
        //Check the time passed 
        double elapsed =difftime(time(NULL), fsm->doorTimeStart); 
        if(elapsed >= DOOR_OPEN_TIME_SEC){
            //Announced so the CC LEDs follow the Sabbath cycle, not just website clicks
            fsmSetDoor(fsm, DOOR_CLOSE);
            fsm->lockedTarget = sabbathMove(fsm);
            //MUST start the pre-move clock here: fsmArrive() zeroes preMoveTimerStart,
            //so without it difftime() measures against the epoch and Sabbath departs
            //the instant the door shuts, skipping the wait entirely.
            fsm->preMoveTimerStart = time(NULL);
            lastPreMovePrint = 0;
            printf("[SBTH] DOOR Closed at Floor %d\n", currentFloor);
        }else{
            return; //Still wating for the door timer 
        }
    }

    //DOOR is Closed, nothing picked (error)
    if(fsm->lockedTarget == 0){
        fsm->lockedTarget = sabbathMove(fsm); 
        fsm->preMoveTimerStart = time(NULL); 
        return; 
    }

    //IF we have a floor picked 
    if(fsm->lockedTarget != 0){
        //Check the time Before moving 
        double waited = difftime(time(NULL), fsm->preMoveTimerStart);
        if(waited < PRE_MOVE_DELAY_SEC){
            if(fsmShouldPrint(&lastPreMovePrint, FSM_DEBUG_PERIOD_SEC)){
                printf("[SBTH] Time to Move to Floor %d (%.0f/%d) elapsed\n", fsm->lockedTarget, waited, PRE_MOVE_DELAY_SEC);
            }
            return;
        }

        lastPreMovePrint = 0; //Wait is over - next one starts fresh
    }

    //Declare the current state (DEPART)
    int target = fsm->lockedTarget;
    fsm->lockedTarget = 0;  
    fsm->previousFloor = currentFloor; 
    fsm->targetFloor = target; 
    fsm->state = STATE_MOVING; 
    fsm->movingTimeStart = time(NULL); 

    printf("[SBTH] State: %s -> MOVING (Floor %d -> Floor %d)\n", fsmStateName((ElevatorSate)(currentFloor -1)), currentFloor, target);

    //Send Command to EC to Move -
    pcanFsmTx(ID_SC_TO_EC, HexFromFloor(target));
    return; 

   }else{
        //We are Moving 
        //Wait for EC_TO_ALL arrivaal 
        double elapsed = difftime(time(NULL), fsm->movingTimeStart); 
        if(elapsed >= MOVING_FALLBACK_SEC){
            printf("[SBTH] No EC Arrival Confirmation (0x101) recieved - Using fallback timeout\n");
            sabbathArrive(fsm, fsm->targetFloor);
        }
    }
}

//One Sabbath step, then mirror the resulting state to the website.
static void sabbathStep(ElevatorFSM *fsm){
    sabbathStepInner(fsm);
    fsmPublishPosition(fsm);
}

//Maintenance Lockout Mode - message ingestion (digital emergency stop)
//Every request coming from HARDWARE is DROPPED: the physical floor-call buttons
//(0x201-0x203) and the in-car car buttons (0x200, floor != 0) are ignored so no
//passenger or floor input can move the car. Only TELEMETRY is honoured - the car's
//own door open/closed reports (ID_CC_TO_SC, floor 0) and the EC arrival
//confirmation (0x101) - so the FSM still tracks the door and its real position.
//The elevator therefore only ever moves in response to website requests, which
//enter separately through fsmPollWebsite()/websiteQueue inside fsmStep().
static void maintenanceProcessMessage(ElevatorFSM *fsm, int id, int data){
    int floor;

    switch (id){
    //Physical floor-call buttons - LOCKED OUT
    case ID_F1_TO_SC:
    case ID_F2_TO_SC:
    case ID_F3_TO_SC:
        //do nothing - hardware request ignored in lockout
        break;

    case ID_CC_TO_SC:
        floor = FloorFromHex(data);
        if(floor == 0){
            //Door open/closed status is telemetry, not a command - still honoured
            fsm->doorClosed = data;
        }
        //In-car floor buttons (floor != 0) are LOCKED OUT
        break;

    case ID_EC_TO_ALL:
        //Arrival confirmation is telemetry - needed so the FSM tracks position
        if(fsm->state == STATE_MOVING){
            floor = FloorFromHex(data);
            if(floor == fsm->targetFloor){
                fsmArrive(fsm, floor);
            }
        }
        break;

    default:
        //do nothing
        break;
    }
}

//Single Supervisory COntroller run LOOp 
void supervisorRun(ElevatorMode startMode){
    ElevatorFSM fsm; 

    int id;
    int data;
    int len; 

    // Force stdout to flush immediately on every print (see fsmRun for rationale).
    setvbuf(stdout, NULL, _IONBF, 0);

    //Clear the screen
    system("@cls||clear");
    printf("\n==== Elevator Supervisory Controller =====\n");
    printf("Starting in %s mode. Modes can also be changed from website\n", modeName(startMode));
    printf("Press CTRL-C to Stop\n\n");

    if(pcanFsmOpen() != 0){
        printf("[FSM] Could not Open CAN Channel - aborting Maintenance Lockout mode \n");
        return;
    }

    audioInit(); //Floor announcements - failure is non fatal, mode just runs silently

    fsmInit(&fsm);
    fsm.modeId = MODE_ELEVATOR; ///FSMapply mode only changes in a request 
    fsmApplyMode(&fsm, startMode); 

    fsmPublishPosition(&fsm); //Record the mode the moment is selected 


    //Maintennace is an e-stop it must not command a mode on entry 
    if(startMode != MODE_MAINTEANCE){
        pcanFsmTx(ID_SC_TO_EC, GO_TO_FLOOR1); 
    }

    g_fsmStop = 0; 
    signal(SIGINT, fsmSigintHandler);

    while(!g_fsmStop){
        //ONE frame per iteration. Draining the whole queue here meant up to 32
        //blocking MariaDB inserts inside a single 200ms tick, which stalled the loop
        //and delayed everything behind it - including the website poll.
        int rc = pcanFsmRxPoll(&id, &data, &len);
        if(rc == 1){
            //LOG FIRST
            fsmLogRxMessage(&fsm, id, data, len);

            //CHange the Way we process Messages according to the mode
            switch (fsm.modeId){
            case MODE_SABBATH:
                sabbathProcessMessages(&fsm, id, data);
                break;

            case MODE_MAINTEANCE:
                maintenanceProcessMessage(&fsm, id, data);
                break;

            default:
                fsmProcessMessage(&fsm, id, data);
                break;
            }
        }

        //Poll the website in EVERY mode. This lived inside fsmStepInner(), which
        //Sabbath never calls, so Sabbath could only be ended with Ctrl-C. Polling
        //before the mode switch means a command read now takes effect this iteration.
        fsmPollWebsite(&fsm);

        //Never Swicth mid Travel it could clear the target and leave the elevator between floors
        if(fsm.pendingMode != fsm.modeId && fsm.state != STATE_MOVING){
            //SWICTH THE MODE 
            fsmApplyMode(&fsm, fsm.pendingMode); 
        }

        if(fsm.modeId == MODE_SABBATH){
            sabbathStep(&fsm);
        }else{
            fsmStep(&fsm);
        }

        usleep(200000); //Responsive enough for the door timer (5 Hz)
    }

    printf("\n[FSM] Stopped by user. Mode: %s, State %s\n", fsm.mode, fsmStateName(fsm.state));
    audioUninit();
    pcanFsmClose();
    signal(SIGINT, SIG_DFL);
}

void fsmRun(void){
    supervisorRun(MODE_ELEVATOR);
}

void sabbathRun(void){
    supervisorRun(MODE_SABBATH); 
}

void maintenanceRun(void){
    supervisorRun(MODE_MAINTEANCE); 
}

// //The FSM RUN 
// void fsmRun(void){
//     ElevatorFSM fsm; 

//     int id; 
//     int data; 
//     int len; 

//     // Force stdout to flush immediately on every print, regardless of whether
//     // this is running on a raw terminal, through a pipe/log/service (which
//     // would otherwise fully-buffer output and delay when prints appear on
//     // screen relative to when they actually happened).
//     setvbuf(stdout, NULL, _IONBF, 0);

//     //Clear the screen 
//     system("@cls||clear"); 
//     printf("\n==== Elevator FSM Mode =====\n");
//     printf("Starting on Floor 1 with the Door OPEN, press CTRL-C to Stop \n\n"); 

//     if(pcanFsmOpen() != 0){
//         printf("[FSM] Could not Open CAN Channel - aborting FSM mode \n");
//         return;
//     }

//     audioInit(); //Floor announcements - failure is non fatal, the FSM just runs silently

//     fsmInit(&fsm); //Keep DB in sync with the FSM initial State
//     fsm.mode = "elevator";
//     fsmPublishPosition(&fsm); //Record the mode the moment it is selected
//     pcanFsmTx(ID_SC_TO_EC, GO_TO_FLOOR1); //Make sure the EC agree we are starting at floor 1

//     g_fsmStop = 0;
//     signal(SIGINT, fsmSigintHandler); // allow ctrl-c to stop the FSM cleanly

//     while(!g_fsmStop){
//         int rc = pcanFsmRxPoll(&id, &data, &len);
//         if(rc == 1){
//             fsmLogRxMessage(&fsm, id, data, len); //Log first - "sensed" means sensed
//             fsmProcessMessage(&fsm, id, data);
//         }

//         fsmStep(&fsm); 

//         usleep(200000); //Responsive enough for 10s door timer 
//     }

//     printf("\n[FSM] Stopped by user. Current State: %s\n", fsmStateName(fsm.state));
//     audioUninit();
//     pcanFsmClose();
//     signal(SIGINT, SIG_DFL); //Restore default Ctrl-C behaviour for the rest of the program
// }

// //Sabbath Mode Run 
// void sabbathRun(void){
//     ElevatorFSM fsm; 

//     int id; 
//     int data; 
//     int len; 

//     // Force stdout to flush immediately on every print, regardless of whether
//     // this is running on a raw terminal, through a pipe/log/service (which
//     // would otherwise fully-buffer output and delay when prints appear on
//     // screen relative to when they actually happened).
//     setvbuf(stdout, NULL, _IONBF, 0);

//     //Clear the screen 
//     system("@cls||clear"); 
//     printf("\n==== Elevator Sabbath Mode =====\n");
//     printf("Starting on Floor 1 with the Door OPEN, press CTRL-C to Stop \n\n"); 


//     if(pcanFsmOpen() != 0){
//         printf("[FSM] Could not Open CAN Channel - aborting FSM mode \n");
//         return; 
//     }

//     audioInit(); //Floor announcements - failure is non fatal, Sabbath mode just runs silently

//     fsmInit(&fsm); //Keep DB in sync with the FSM initial State
//     fsm.mode = "sabbath";

//     //Due to Sabbath we need to change some states
//     fsm.doorClosed = DOOR_OPEN;
//     fsm.doorTimeStart = time(NULL);

//     fsmPublishPosition(&fsm); //Record the mode the moment it is selected
//     pcanFsmTx(ID_SC_TO_EC, GO_TO_FLOOR1); //Make sure the EC agree we are starting at floor 1

//     g_fsmStop = 0;
//     signal(SIGINT, fsmSigintHandler); // allow ctrl-c to stop the FSM cleanly

//     while(!g_fsmStop){
//         int rc = pcanFsmRxPoll(&id, &data, &len);
//         if(rc == 1){
//             //Sabbath ignores every request, but the frame was still sensed - log it
//             fsmLogRxMessage(&fsm, id, data, len);
//             sabbathProcessMessages(&fsm, id, data);
//         }
        
//         //Start on Sabath Mode. It should not handle any inputs
//         sabbathStep(&fsm); 
//         usleep(200000); //Responsive enough for 10s door timer 
//     }

//     printf("\n[FSM] Stopped by user. Current State: %s\n", fsmStateName(fsm.state));
//     audioUninit();
//     pcanFsmClose();
//     signal(SIGINT, SIG_DFL); //Restore default Ctrl-C behaviour for the rest of the program

// }



// //Maintenance Lockout Mode Run - digital emergency stop.
// //Reuses the normal FSM brain (fsmStep / fsmArrive / fsmPollWebsite); the only
// //difference from fsmRun() is maintenanceProcessMessage(), which drops every
// //hardware request before it can reach a queue. The car holds in place with the
// //door OPEN and does not move until a website request arrives.
// void maintenanceRun(void){
//     ElevatorFSM fsm;

//     int id;
//     int data;
//     int len;

//     // Force stdout to flush immediately on every print (see fsmRun for rationale).
//     setvbuf(stdout, NULL, _IONBF, 0);

//     //Clear the screen
//     system("@cls||clear");
//     printf("\n==== Elevator Maintenance Lockout Mode =====\n");
//     printf("Digital EMERGENCY STOP: hardware buttons are LOCKED OUT.\n");
//     printf("The elevator only moves on WEBSITE requests. Press CTRL-C to Stop\n\n");

//     if(pcanFsmOpen() != 0){
//         printf("[FSM] Could not Open CAN Channel - aborting Maintenance Lockout mode \n");
//         return;
//     }

//     audioInit(); //Floor announcements - failure is non fatal, mode just runs silently

//     fsmInit(&fsm);
//     fsm.mode = "maintenance";

//     //Digital e-stop: hold in place with the door OPEN and do NOT command a move on
//     //entry. With every hardware queue empty, the car stays put until a website
//     //request lands in the websiteQueue.
//     fsm.doorClosed = DOOR_OPEN;
//     fsm.doorTimeStart = time(NULL);

//     fsmPublishPosition(&fsm); //Record the mode the moment it is selected

//     g_fsmStop = 0;
//     signal(SIGINT, fsmSigintHandler); // allow ctrl-c to stop cleanly

//     while(!g_fsmStop){
//         int rc = pcanFsmRxPoll(&id, &data, &len);
//         if(rc == 1){
//             //Lockout drops every hardware button, but the frame was still sensed - log it
//             fsmLogRxMessage(&fsm, id, data, len);
//             maintenanceProcessMessage(&fsm, id, data);
//         }

//         fsmStep(&fsm);

//         usleep(200000); //Responsive enough for the door timer
//     }

//     printf("\n[FSM] Maintenance Lockout stopped by user. Current State: %s\n", fsmStateName(fsm.state));
//     audioUninit();
//     pcanFsmClose();
//     signal(SIGINT, SIG_DFL); //Restore default Ctrl-C behaviour for the rest of the program
// }

