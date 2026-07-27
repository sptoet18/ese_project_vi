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
int db_getFloorNum();               // returns the floor, or -1 if unavailable
int db_setFloorNum(int floorNum);   // 0 on success, -1 on failure

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
