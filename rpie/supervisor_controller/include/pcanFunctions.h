#ifndef PCAN_FUNCTIONS
#define PCAN_FUNCTIONS


// Defines
// ***********************************************************************************************************
#define PCAN_RECEIVE_QUEUE_EMPTY        0x00020U  	// Receive queue is empty
#define PCAN_NO_ERROR               	0x00000U  	// No error 

// Elevator project specific 
#define ID_SC_TO_EC  0x100	// ID for messages from Supervisory controller to elevator controller
#define ID_EC_TO_ALL 0x101	// ID for messages from Elevator controller to all other nodes
#define ID_CC_TO_SC  0x200	// ID for messages from Car controller to supervisory controller 
#define ID_F1_TO_SC  0x201	// ID for messages from floor 1 controller to supervisory controller
#define ID_F2_TO_SC  0x202	// ID for messages from floor 2 controller to supervisory controller
#define ID_F3_TO_SC  0x203	// ID for messages from floor 3 controller to supervisory controller	

#define GO_TO_FLOOR1 0x05 // Go to floor 1
#define GO_TO_FLOOR2 0x06	// Go to floor 2
#define GO_TO_FLOOR3 0x07	// Go to floor 3

#define GO_TO_FLOOOR1 0x01 // Go to floor 1
#define GO_TO_FLOOOR2 0x02	// Go to floor 2
#define GO_TO_FLOOOR3 0x03	// Go to floor 3

#define DOOR_CLOSE 0x08
#define DOOR_OPEN 0x09



// Function declarations
int pcanTx(int id, int data);
int pcanRx(int num_msgs);

// --- Decode helpers (used by the RX/TX switch-case printouts) ---
// Exposed in case other modules want human-readable names too.
const char* decodeSenderName(int id);
const char* decodeRecipientName(int id);
const char* decodeMsgType(int id, int data);

// --- Persistent-handle, non-blocking CAN I/O for continuous FSM use ---
// Unlike pcanTx()/pcanRx() (which open/close the channel and block on every
// call), these keep one CAN handle open so the FSM loop can poll for
// messages AND service a door timer in the same loop without stalling.
int  pcanFsmOpen(void);                                   // open + init channel once; returns 0 on success, -1 on failure
void pcanFsmClose(void);                                  // close the persistent channel
int  pcanFsmTx(int id, int data);                          // non-blocking send (prints TX decode); returns 0 on success
int  pcanFsmRxPoll(int *outId, int *outData, int *outLen); // non-blocking receive; returns 1 if a message was read, 0 if none waiting, -1 on error



#endif
