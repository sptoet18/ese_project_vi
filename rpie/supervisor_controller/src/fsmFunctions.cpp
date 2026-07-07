//AUTHOR: Emiliano Perez Pellicer 
//DATE: 03/07/2026
//COMMENTS: This is a finite state machine 
//VERSION: 1.0 
#include "../include/pcanFunctions.h"
#include "../include/mainFunctions.h"
#include "../include/databaseFunctions.h"
#include "../include/fsmFuntions.h"

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

//Initial Condition 
void fsmInit(ElevatorFSM *fsm){
    fsm->state = STATE_FLOOR1; 
    //Add the queues 
    fsm->carQueue = std::queue<int>();
    fsm->floorQueue = std::queue<int>();
    fsm->websiteQueue = std::queue<int>();
    

    fsm->doorClosed =0; //Door Starts Open
    fsm->previousFloor = 1; 
    fsm->targetFloor = 1;
    fsm->lastDbfloor = 1;
    fsm->lastWebsitePoll = time(NULL);  
    fsm->doorTimeStart = time(NULL); 
    fsm->movingTimeStart = 0; 
    fsm->collectTimerStart = 0; //window for listening (elevator door is open right now )
    fsm->lockedTarget =0;
    fsm->preMoveTimerStart = 0; 
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

//Polls the DB for a website requedted floor at most once every Macro preivously define 
static void fsmPollWebsite(ElevatorFSM *fsm){
    double sinceLastPoll = difftime(time(NULL), fsm->lastWebsitePoll); 

    if(sinceLastPoll < WEBSITE_POLL_SEC){
        return; 
    }
    fsm->lastWebsitePoll = time(NULL); 

    //int dbFloor = db_getFloorNum();
    int dbFloor = fsm->lastDbfloor;
    if(dbFloor == fsm->lastDbfloor){
        return; //No change since we last looked or is the same 
    }
    fsm->lastDbfloor = dbFloor; 

    if(dbFloor >= 1 && dbFloor <= 3 && !queueContainsFloor(fsm->websiteQueue, dbFloor)){
        fsm->websiteQueue.push(dbFloor);
        printf("[FSM] Website request queued (Priority 3): FLOOR %d\n", dbFloor); 
    }
}


//Whenever the elevator arrives at a floor 
//EC_TO_ALL confirmaton, or from the MoVING fall back timeput 
static void fsmArrive(ElevatorFSM *fsm, int floor){
    fsm->state = (ElevatorSate)(floor - 1); //STATE FLOOR - 1 
    fsm->doorClosed  = 0; 
    fsm->doorTimeStart = time(NULL);
    fsm->collectTimerStart = 0; // No listening window until the door closes again 
    fsm->lockedTarget = 0; 
    fsm->preMoveTimerStart = 0; 
    
    fsmConsumeRequestsForFloor(fsm, floor); 
    
    //db_setFloorNum(floor); //Keep the website/Db in sync with the real  elevator position 
    fsm->lastDbfloor = floor; //Next website poll doesn't re que our own update 

    printf("[FSM] Arrived at Floor %d - door OPEN(%ds Timer started)\n", floor, DOOR_OPEN_TIME_SEC);
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
        if(!queueContainsFloor(fsm->carQueue, floor)){
            fsm->carQueue.push(floor);
            printf("[FSM] Car Request Detected (Prioptity 1): FLOOR %d\n", floor); 
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
static void fsmStep(ElevatorFSM *fsm){
    static time_t lastCollectingPrint = 0; 

    fsmPollWebsite(fsm); //Queues accumulate even while moving - Queed for larer 


    if(fsm->state != STATE_MOVING){
        //Register the current floor 
        int currentFloor = (int)fsm->state + 1; //State Floor1(0) -> 1

        //DOOR IS OPEN 
        if(!fsm->doorClosed){
            //Check the time passed 
            double elapsed = difftime(time(NULL), fsm->doorTimeStart); 
            if(elapsed >= DOOR_OPEN_TIME_SEC){
                fsm->doorClosed = 1; 
                fsmConsumeRequestsForFloor(fsm, currentFloor); //Requesrts at this floor are now beind done 
                fsm->collectTimerStart =0; //re start listening window 
                fsm->lockedTarget =0;
                printf("[FSM] Door CLOSED at Floor %d\n", currentFloor);
            
            }else{
                return; //Still wating for the door timer 
            }
        }
        if(fsm->lockedTarget != 0){
            double waited = difftime(time(NULL), fsm->preMoveTimerStart); 
            if(waited < PRE_MOVE_DELAY_SEC){
                time_t nowSec = time(NULL); 
                if(nowSec != lastCollectingPrint ){ //resuse the same once/sec
                    lastCollectingPrint = nowSec; 
                    printf("[FSM] DEBUG: Wasiting to move to FLOOR %d (%.0f/%d) elapsed\n", fsm->lockedTarget, waited, PRE_MOVE_DELAY_SEC);
                }
                return; //Still waiting 
            }

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
        if(fsm->collectTimerStart == 0){
            fsm->collectTimerStart = time(NULL);
            lastCollectingPrint = 0; 
            printf("[FSM] Listening for requests for %d seconds before choosing Next Stop\n",REQUEST_COLLECTION_SEC);
            return; 
        }

        //NOW COLLECTING REQUEST 
        double collecting = difftime(time(NULL), fsm->collectTimerStart); 
        if(collecting < REQUEST_COLLECTION_SEC){
            time_t nowSec = time(NULL); 
            if(nowSec != lastCollectingPrint ){ //resuse the same once/sec
                lastCollectingPrint = nowSec; 
                printf("[FSM] DEBUG: still collecting");
            }
            return; //Still on the collecting window - keep collecting
        }

        //LISTENING WINDOW IS OVER PICK NEXT TARGET 

        int target = fsmPeekPriotity(fsm, currentFloor); 
        if(target != 0 && target != currentFloor){
            fsmConsumeRequestsForFloor(fsm, target); //locked in now - remove from queues 
            fsm->lockedTarget = target; 
            fsm->preMoveTimerStart = time(NULL); 
            fsm->collectTimerStart =0; //reset for next stop 

            //Print what got locked in - actual move happens after Phase B (see top of this function)
            printf("[FSM] Locked FLOOR %d - waiting %d seconds before moving\n", target, PRE_MOVE_DELAY_SEC); 
            
        }else{
            //Nothing then restarts timer 
            fsm->collectTimerStart = time(NULL); 
        }

    }else{
        //We are MOVING 
        //Either wait for a real EC_TO_ALL arrival messafe (Handleld in fsmProcessMessage ) if not recieved use timer 

        double elapsed = difftime(time(NULL), fsm->movingTimeStart); 
        if (elapsed >= MOVING_FALLBACK_SEC){
            printf("[FSM] No EC Arrival confirmation (0x101) recieved - using fallback timeout\n");
            fsmArrive(fsm, fsm->targetFloor); 
        }
    }
}

//The FSM RUN 
void fsmRun(void){
    ElevatorFSM fsm; 

    int id; 
    int data; 
    int len; 

    // Force stdout to flush immediately on every print, regardless of whether
    // this is running on a raw terminal, through a pipe/log/service (which
    // would otherwise fully-buffer output and delay when prints appear on
    // screen relative to when they actually happened).
    setvbuf(stdout, NULL, _IONBF, 0);

    //Clear the screen 
    system("@cls||clear"); 
    printf("\n==== Elevator FSM Mode =====\n");
    printf("Starting on Floor 1 with the Door OPEN, press CTRL-C to Stop \n\n"); 

    if(pcanFsmOpen() != 0){
        printf("[FSM] Could not Open CAN Channel - aborting FSM mode \n");
        return; 
    }

    fsmInit(&fsm); //Keep DB in sync with the FSM initial State 
    //db_setFloorNum(1); 
    pcanFsmTx(ID_SC_TO_EC, GO_TO_FLOOR1); //Make sure the EC agree we are starting at floor 1

    g_fsmStop = 0; 
    signal(SIGINT, fsmSigintHandler); // allow ctrl-c to stop the FSM cleanly 

    while(!g_fsmStop){
        int rc = pcanFsmRxPoll(&id, &data, &len); 
        if(rc == 1){
            fsmProcessMessage(&fsm, id, data); 
        }

        fsmStep(&fsm); 

        usleep(200000); //Responsive enough for 10s door timer 
    }

    printf("\n[FSM] Stopped by user. Current State: %s\n", fsmStateName(fsm.state)); 
    pcanFsmClose(); 
    signal(SIGINT, SIG_DFL); //Restore default Ctrl-C behaviour for the rest of the program
}