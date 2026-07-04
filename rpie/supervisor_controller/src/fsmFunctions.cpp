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
    int f; 

    fsm->state = STATE_FLOOR1; 
    //Loocp Acrros the floors 
    for(f = 0; f < 3; f++){
        fsm->floorRequest[f] = 0; 
        fsm->carRequest[f] =0; 
    }

    fsm->doorClosed =0; //Door Starts Open
    fsm->previousFloor = 1; 
    fsm->targetFloor = 1; 
    fsm->doorTimeStart = time(NULL); 
    fsm->movingTimeStart = 0; 
}

//Go to next floor once the door is closed (Passenger in car) first car request then floor class 
static int fsmFindNextRequest(ElevatorFSM *fsm, int currentFloor){
    int f; 

    //Check for rhe Car Controller Request 
    for (f = 1; f <= 3; f++){
        if(f != currentFloor && fsm->carRequest[f - 1]){
            return f; 
        }
    }

    //Checl for the Current Floor and Floor Request 
    for (f = 1; f <= 3; f++){
        if(f != currentFloor && fsm->floorRequest[f -1]){
            return f; 
        }
    }
    return 0; 
}

//Whenever the elevator arrives at a floor 
//EC_TO_ALL confirmaton, or from the MoVING fall back timeput 
static void fsmArrive(ElevatorFSM *fsm, int floor){
    fsm->state = (ElevatorSate)(floor - 1); //STATE FLOOR - 1 
    fsm->doorClosed  = 0; 
    fsm->doorTimeStart = time(NULL);
    fsm->floorRequest[floor - 1] = 0; //This floor outstanding request are now satisfied 
    fsm->carRequest[floor - 1] = 0; 

    db_setFloorNum(floor); //Keep the website/Db in sync with the real  elevator position 

    printf("[FSM] Arrived at Floor %d - door OPEN(%ds Timer started)\n", floor, DOOR_OPEN_TIME_SEC);
}

//Applies one incoming CAN message to the FSM's request flags / arrival detection 
static void fsmProcessMessage(ElevatorFSM *fsm, int id, int data){
    int floor; 

    switch (id){
    case ID_F1_TO_SC:
        fsm->floorRequest[0] = 1; 
        printf("[FSM] Floor-call request detected: FLOOR 1\n");
        break;
    
    case ID_F2_TO_SC:
        fsm->floorRequest[1] = 1; 
        printf("[FSM] Floor-call request detected: FLOOR 2\n");
        break;
    
    case ID_F3_TO_SC:
        fsm->floorRequest[2] = 1; 
        printf("[FSM] Floor-call request detected: FLOOR 3\n");
        break;
    
    case ID_CC_TO_SC:
        floor = FloorFromHex(data); 
        fsm->carRequest[floor - 1] = 1; 
        printf("[FSM] Car Request Detected: FLOOR %d\n");
        break;

    default:
    //do nothing 
        break;
    }
}

//Runs one FSM evaluation stem (door timer, request shcedueling, moving fallback )
static void fsmStep(ElevatorFSM *fsm){
    if(fsm->state != STATE_MOVING){
        //Register the current floor 
        int currentFloor = (int)fsm->state + 1; //State Floor1(0) -> 1

        //DOOR IS OPEN 
        if(!fsm->doorClosed){
            //Check the time passed 
            double elapsed = difftime(time(NULL), fsm->doorTimeStart); 
            if(elapsed >= DOOR_OPEN_TIME_SEC){
                fsm->doorClosed = 1; 
                fsm->floorRequest[currentFloor - 1]=0; ///Request at this floor is now served 
                fsm->carRequest[currentFloor - 1]=0; 
                printf("[FSM] Door CLOSED at Floor %d\n", currentFloor);
            
            }else{
                return; //Still wating for the door timer 
            }
        }

        //DOOR IS CLOSED 
        int target = fsmFindNextRequest(fsm, currentFloor); 
        if(target != 0 && target != currentFloor){
            fsm->previousFloor = currentFloor; 
            fsm->targetFloor = target; 
            fsm->state = STATE_MOVING; 
            fsm->movingTimeStart = time(NULL); 

            //Print Where is going 
            printf("[FSM] State: %s -> MOVING (Floor %d -> Floor %d)\n", fsmStateName((ElevatorSate)(currentFloor - 1)), currentFloor, target); 

            //Send command to EC to move 
            pcanTx(ID_SC_TO_EC, HexFromFloor(target)); 
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

    //Clear the screen 
    system("@cls||clear"); 
    printf("\n==== Elevator FSM Mode =====\n");
    printf("Starting on Floor 1 with the Door OPEN, press CTRL-C to Stop \n\n"); 

    if(pcanFsmOpen() != 0){
        printf("[FSM] Could not Open CAN Channel - aborting FSM mode \n");
        return; 
    }

    fsmInit(&fsm); //Keep DB in sync with the FSM initial State 
    db_setFloorNum(1); 
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