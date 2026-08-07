#ifndef DB_FUNCTIONS

#define DB_FUNCTIONS

// --- Connection lifecycle ---
// The firmware used to open and tear down a full MySQL connection on EVERY call,
// with no exception handling at all - an uncaught sql::SQLException calls
// std::terminate and kills the whole program. That was survivable when the only
// writes were a couple per arrival; it is not, now that we insert a row for every
// CAN frame sensed at the FSM loop rate. One connection is opened here and reused,
// and every function below catches its own errors and returns a code instead.
int  db_open(void);   // connect + cache the known CAN node ids; 0 on success, -1 on failure
void db_close(void);  // tear down the shared connection

// --- Legacy phase-1 elevatorNetwork table (website request channel) ---
//Website Command Channels 
typedef enum{
    WEB_CMD_NONE = 0, 
    WEB_CMD_FLOOR_CAR,  //CarController 
    WEB_CMD_FLOOR_REQ,  //FloorController 
    WEB_CMD_MODE //Mode of operation 
} WebCmdKind; 

typedef enum{
    WEB_MODE_ELEVATOR = 0x10,
    WEB_MODE_SABBATH = 0x11, 
    WEB_MODE_MAINTEANCE = 0x12
} WebModeCode; 

typedef struct {
    WebCmdKind kind;
    int floor; //1,2,3 for the floor_* kinds  
    int mode; //WebmodeCode
} WebCommand; 

//Will set the cursor at the newest command 
int db_initWebsiteCursor(void); 

//Drains the cursos but returns how many commands were written 
int db_getWebsiteCommands(WebCommand *out, int maxOut);               // returns the floor, or -1 if unavailable

// --- Modern tables read by the website ---

// True if canId exists in can_node.can_id. can_transaction.sent_by is a FOREIGN KEY
// onto that column, so inserting an unknown id would throw; callers use this to skip.
bool db_isKnownCanNode(int canId);

// Records one CAN frame sensed on the bus. Returns 0 on success, -1 if the insert
// was skipped (unknown node) or failed.
int db_logCanTransaction(int canId, int data, const char *message,
                         int currentFloor, int lastFloor);

// Appends a row to elevator_position. mode MUST be one of "elevator", "sabbath" or
// "maintenance" - the column is an ENUM and anything else stores as ''.
int db_insertElevatorPosition(int currentFloor, int lastFloor,
                              int isMoving, int isClosed, const char *mode);

// Reads the newest elevator_position row. Falls back to floor 1 when the table is
// empty. Returns 0 on success, -1 on failure (outputs still set to the fallback).
int db_getLatestPosition(int *outCurrentFloor, int *outLastFloor);

#endif
