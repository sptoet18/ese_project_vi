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

#define DOOR_OPEN_TIME_SEC 5///Door stayys open for 10 sec before closing
//Last-resort recovery for a LOST 0x101 arrival frame, NOT a measure of travel time.
//If it fires before the real arrival, fsmArrive() fabricates one and the FSM desyncs.
#define MOVING_FALLBACK_SEC 8
//How often to poll the DB for website commands. In MILLISECONDS on a monotonic
//clock: the old time_t/difftime() version could only count whole seconds, so the
//real interval drifted between 1s and 2s depending on when the row was inserted.
#define WEBSITE_POLL_MS 300
#define REQUEST_COLLECTION_SEC 5
#define PRE_MOVE_DELAY_SEC 5

//How often an in-progress phase repeats its debug line. The first tick always
//prints; this only throttles repeats, so demo timers stay visible without flooding.
//2s gives ~3 lines per 5s window - 3s read as a dead terminal, 1s was the flood.
#define FSM_DEBUG_PERIOD_SEC 2

typedef enum{
    MODE_ELEVATOR = 0, 
    MODE_SABBATH = 1, 
    MODE_MAINTEANCE = 2
}ElevatorMode;

typedef struct {
    ElevatorSate state; 

    ///Priority queues 
    std::queue<int> carQueue; // Priotity 1 
    std::queue<int> floorQueue; //Priotity 2 
    std::queue<int> websiteQueue; //Priority 3 

    int doorClosed; //Holds DOOR_CLOSE (0x08) or DOOR_OPEN (0x09) - NOT a 0/1 flag
    int previousFloor; //Las floor the elevator was AT before it started moving (1,2,3)
    int targetFloor; //Destinantion floor while state == STATE_MOVING (1,2,3)

    ElevatorMode modeId;  //Machine mode 
    ElevatorMode pendingMode;  //Webiste requested mode 

    long long lastWebsitePollMs; //Monotonic ms timestamp of the last website poll

    time_t doorTimeStart; //Timestamp when the door was last open 
    time_t collectTimerStart; //Time to listen for request 
    time_t movingTimeStart; //Timestamp when the elevator started moving (foor the fallback timeout)

    int lockedTarget;
    time_t preMoveTimerStart;

    //Which mode the supervisory controller is running. Written straight into
    //elevator_position.mode, so it MUST match that ENUM: "elevator", "sabbath"
    //or "maintenance".
    const char *mode;

    //Snapshot of the last row published to elevator_position. The run loops tick at
    //5 Hz, so we only insert when one of these actually changes.
    int pubFloor;
    int pubLast;
    int pubMoving;
    int pubClosed;
    const char *pubMode;

}ElevatorFSM;

//Functions declarations 
void fsmInit(ElevatorFSM *fsm); 
void fsmRun(void); 
const char* fsmStateName(ElevatorSate s);

//Functions Declarations - Sabbath Mode
void sabbathRun(void);

//Functions Declarations - Maintenance Lockout Mode (digital emergency stop)
void maintenanceRun(void);

//Functiont to tun all at once (Same process)
void supervisorRun(ElevatorMode startMode);
const char* modeName(ElevatorMode m);


#endif