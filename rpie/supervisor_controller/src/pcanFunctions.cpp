#include "../include/pcanFunctions.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>  
#include <errno.h>
#include <unistd.h> 
#include <signal.h>
#include <string.h>
#include <fcntl.h>    					// O_RDWR
#include <unistd.h>
#include <ctype.h>
#include <libpcan.h>   					// PCAN library


// Globals
// ***********************************************************************************************************
HANDLE h;
HANDLE h2;
TPCANMsg Txmsg;
TPCANMsg Rxmsg;
DWORD status;

//persisten Handle + message buffer used by the non-blocking FSM I/O functions 
static HANDLE hfsm = NULL; 
static TPCANMsg TxmsgFsm;
static TPCANMsg RxmsgFsm;

// Code
// ***********************************************************************************************************

//Decode helpers 
// ***********************************************************************************************************
//Maps a CAN ID to the Human-readble name of the node that TRANSMITS on 
//That ID per the "Elevator system"
const char* decodeSenderName(int id){
	switch (id){
	case ID_SC_TO_EC:
		return "Supervisory Controller (SC)"; 
		break;
	
	case ID_EC_TO_ALL:
		return "Elevator Controller (EC)"; 
		break;
	
	case ID_CC_TO_SC:
		return "Car Controller (CC)"; 
		break;
	
	case ID_F1_TO_SC:
		return "Floor 1 Controller (F1)";
		break;
	
	case ID_F2_TO_SC:
		return "Floor 2 Controller (F2)";
		break;
	
	case ID_F3_TO_SC:
		return "Floor 3 Controller (F3)"; 
		break;
	
	default:
		return "UNKNOWN"; 
		break;
	}
}


//Map a CAN ID to the intended Reciever 
const char* decodeRecipientName(int id){
	switch (id){
	case ID_SC_TO_EC:
		return "Elevator Controller (EC)";
		break;
	
	case ID_EC_TO_ALL:
		return "All Nodes"; 
		break;
	
	case ID_CC_TO_SC:
		return "Supervisory Controller (SC)"; 
		break;
	
	case ID_F1_TO_SC:
		return "Supervisory Controller (SC)"; 
		break;
	
	case ID_F2_TO_SC:
		return "Supervisory Controller (SC)"; 
		break;
	
	case ID_F3_TO_SC:
		return "Supervisory Controller (SC)"; 
		break;
	
	default:
		return "UNKNOWN"; 
		break;
	}
}

//Map a data byte value to the type of request/status it represents 
const char* decodeMsgType(int data){
	switch (data){
	case	GO_TO_FLOOR1:
		return "FLOOR 1"; 
		break;
	
	case	GO_TO_FLOOR2:
		return "FLOOR 2"; 
		break;

	case	GO_TO_FLOOR3:
		return "FLOOR 3"; 
		break;
	
	default:
		return "UNKNOWN"
		break;
	}
}


//Print a full readable decode of a RECIEVED CAN MESSAGE
//Sender, lenght, and type 
static void printRxDecoded(int id, int len, int data){
	switch (id){
	case ID_EC_TO_ALL: //0x101: EC -> ALL (Position/Status report)
		printf(" [RX] Sender: %-30s LEN: %d Type: EC Status/Position Report -> %s\n", decodeSenderName(id), len, decodeMsgType(data)); 
		break;
	
	case ID_CC_TO_SC: //0x200: CC -> SC (Request from inside the car)
		printf(" [RX] Sender: %-30s LEN: %d Type: Car Request (inside elevator) -> %s\n", decodeSenderName(id), len, decodeMsgType(data)); 
		break;

	case ID_F1_TO_SC: //0x201: Floor 1 call button -> SC
		printf(" [RX] Sender: %-30s LEN: %d Type: Floor Car Request -> %s\n", decodeSenderName(id), len, decodeMsgType(data)); 
		break;

	case ID_F2_TO_SC: //0x202: Floor 2 call button -> SC 
		printf(" [RX] Sender: %-30s LEN: %d Type: Floor Car Request -> %s\n", decodeSenderName(id), len, decodeMsgType(data)); 
		break;

	case ID_F3_TO_SC: //0x203: Floor 3 call button -> SC 
		printf(" [RX] Sender: %-30s LEN: %d Type: Floor Car Request -> %s\n", decodeSenderName(id), len, decodeMsgType(data)); 
		break;

	case ID_SC_TO_EC: //0x100: SC -> EC (Only Expected if SC hear its own Tx Echo back)
		printf(" [RX] Sender: %-30s LEN: %d Type: Command to EC -> %s\n", decodeSenderName(id), len, decodeMsgType(data)); 
		break;

	
	default:
		printf(" [RX] Sender: Unknown (ID 0x%x) LEN %d Type: Unrecognized message\n", id, len); 
		break;
	}
}

//Print Full human-reable decode of a TX CAN Message 
//What SC is sendinf, lenght, WHO and TYPE 
static void printTxDecoded(int id, int len, int data){
	switch (id){
	case ID_SC_TO_EC: //0x100:  The only ID the SC is defined to transmit on 
		printf(" [TX] From Supervisory Cntroller (SC) To: %-25s LEN: %d Type: Command -> GO TO %s\n", decodeRecipientName(id), len, decodeMsgType(data));
		break;
	
	default:
		printf("  [TX] From: Supervisory Controller (SC)  To: Unknown (ID 0x%X)  LEN: %d  Type: Unrecognized command (0x%02X)\n",id, len, data); 
		break;
	}
}

// Functions
// *****************************************************************
int pcanTx(int id, int data){
	h = LINUX_CAN_Open("/dev/pcanusb32", O_RDWR);		// Open PCAN channel

	// Initialize an opened CAN 2.0 channel with a 125kbps bitrate, accepting standard frames
	status = CAN_Init(h, CAN_BAUD_125K, CAN_INIT_TYPE_ST);

	// Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h);

	// Set up message
	Txmsg.ID = id; 	
	Txmsg.MSGTYPE = MSGTYPE_STANDARD; 
	Txmsg.LEN = 1; 
	Txmsg.DATA[0] = data; 

	sleep(1);  
	status = CAN_Write(h, &Txmsg);

	//Tx Switch-case: Print what the SC is sending to whom, lenght and tyoe 
	printTxDecoded(id, (int)Txmsg.LEN, data);

	// Close CAN 2.0 channel and exit	
	CAN_Close(h);

	return(status == PCAN_NO_ERROR) ? 0 : -1;  //Conditional Expersion IF NO ERROR return 0 if not -1 
}

int pcanRx(int num_msgs){
	int i = 0;

	// Open a CAN channel 
	h2 = LINUX_CAN_Open("/dev/pcanusb32", O_RDWR);

	// Initialize an opened CAN 2.0 channel with a 125kbps bitrate, accepting standard frames
	status = CAN_Init(h2, CAN_BAUD_125K, CAN_INIT_TYPE_ST);

	// Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);

	// Clear screen to show received messages
	system("@cls||clear");

	// receive CAN message  - CODE adapted from PCAN BASIC C++ examples pcanread.cpp
	printf("\nReady to receive message(s) over CAN bus\n");
	
	// Read 'num' messages on the CAN bus
	while(i < num_msgs) {
		while((status = CAN_Read(h2, &Rxmsg)) == PCAN_RECEIVE_QUEUE_EMPTY){
			sleep(1);
		}
		if(status != PCAN_NO_ERROR) {						// If there is an error, display the code
			printf("Error 0x%x\n", (int)status);
			//break;
		}
										
		if(Rxmsg.ID != 0x01 && Rxmsg.LEN != 0x04) {		// Ignore status message on bus	
			printf("  - R ID:%4x LEN:%1x DATA:%02x \n",	// Display the CAN message
				(int)Rxmsg.ID, 
				(int)Rxmsg.LEN,
				(int)Rxmsg.DATA[0]);
			
			//RX Switch case: Print who sent it , the lenght, and the type of request it is 
			printRxDecoded((int)Rxmsg.ID, (int)Rxmsg.LEN,, (int)Rxmsg.DATA[0]);

		i++;
		}

	}
	

	// Close CAN 2.0 channel and exit	
	CAN_Close(h2);
	//printf("\nEnd Rx\n");
	return ((int)Rxmsg.DATA[0]);						// Return the last value received
}

/// Persistent-handle, non-blocking CAN I/O for the FSM LOOP 
int pcanFsmOpen(void){
	hFsm = LINUX_CAN_Open("/dev/pcanusb32", O_RDWR);
	if(!hFsm){
		printf("[FSM] ERROR: Could not open /dev/pcanusb32\n");
		return -1; 
	}
	status = CAN_Init(hFsm, CAN_BAUD_125K, CAN_INIT_TYPE_ST);
	status = CAN_Status(hFsm); //Clear Channel before use, as in pcanTx/Rx
	return 0;  
}

void pcanFsmClose(void){
	if(hFsm){
		CAN_Close(hFsm);
		hFsm = NULL; 
	}
}

int pcanFsmTx(int id, int data){
	if(!hFsm){
		return -1; 
	}

	TxmsgFsm.ID = id; 
	TxmsgFsm.MSGTYPE = MSGTYPE_STANDARD; 
	TxmsgFsm.LEN = 1; 
	TxmsgFsm.DATA[0] = data; 

	status = CAN_Write(hFsm, &TxmsgFsm); 

	printTxDecoded(id, (int)TxmsgFsm.LEN, data); //Same TX Csae 

	return(status == PCAN_NO_ERROR) ? 0 : -1; 
}

int pcanFsmRxPoll(int *outId int *outData, int *outLen){
	if (!hFsm){
		return -1; 
	}

	status = CAN_Read(hFsm, &RxmsgFsm);

	if(status == PCAN_RECEIVE_QUEUE_EMPTY){
		return 0; //Nothing wating, no error just no message 
	}

	if(status != PCAN_NO_ERROR){
		printf("[FSM] CAN Read Error 0x%x\n", (int)status);
		return -1; 
	}

	if(RxmsgFsm.ID == 0x01 && RxmsgFsm.LEN == 0x04){
		return 0; //ignore bus status frame, same filtrer as pcanRx
	}

	*outId = (int)RxmsgFsm.ID;
	*outData = (int)RxmsgFsm.DATA[0]; 
	*outLen = (int)RxmsgFsm.LEN; 

	printRxDecoded(*outId, *outLen, *outData); //Same Rx Switch case
	
	return 1; 
}