#ifndef FSM_FUNCTIONS
#define FSM_FUNCTIONS

#include <time.h>
#include <queue>

/*  
Elevator Finite State Machone 

States(4):
Elevator is either sitting at one of the 3 floors
(Door is open, closing or closed )
Or Moving 

Requests are tracked separately for: 
-FloorRequest outside the car controller in the individuals floor s
-Carrequest inise the Car COntroller 


Door behavipur: Whenever the elevator is  sitting at a floor, the door is
// OPEN for DOOR_OPEN_TIME_SEC seconds (timer) before auto-closing. Once
// closed, if there is a pending request for a different floor, the FSM
// commands the Elevator Controller (SC -> EC, ID 0x100) to move there.

*/


typedef enum {
    STATE_FLOOR1 = 0, //floor num - 1 used as an array input 
    STATE_FLOOR2 = 1, 
    STATE_FLOOR3 = 2, 
    STATE_MOVING = 3
} ElevatorSate; 

#define DOOR_OPEN_TIME_SEC 10 ///Door stayys open for 10 sec before closing 
#define MOVING_FALLBACK_SEC 8 ///
#define WEBSITE_POLL_SEC 1 //How often to poll the DB for a new websirte requested floor 

typedef struct {
    ElevatorSate state; 

    ///Priority queues 
    std::queue<int> carQueue; // Priotity 1 
    std::queue<int> floorQueue; //Priotity 2 
    std::queue<int> websiteQueue; //Priority 3 

    int doorClosed; // 0 = door open, 1 = door open 
    int previousFloor; //Las floor the elevator was AT before it started moving (1,2,3)
    int targetFloor; //Destinantion floor while state == STATE_MOVING (1,2,3)

    int lastDbfloor;     ///last currentfloor read/written value to rghe DB to detect new website requests 
    time_t lastWebsitePoll; //Time Stamp for last poll 

    time_t doorTimeStart; //Timestamp when the door was last open 
    time_t movingTimeStart; //Timestamp when the elevator started moving (foor the fallback timeout)

}ElevatorFSM; 

//Functions declarations 
void fsmInit(ElevatorFSM *fsm); 
void fsmRun(void); 
const char* fsmStateName(ElevatorSate s);

#endif