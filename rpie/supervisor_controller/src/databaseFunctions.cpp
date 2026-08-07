// Includes required (headers located in /usr/include)
#include "../include/databaseFunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <set>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace std;

// Connection details - kept in ONE place instead of being repeated in every function
// ***********************************************************************************
static const char *DB_HOST   = "tcp://127.0.0.1:3306";
static const char *DB_USER   = "Emiliano";
static const char *DB_PASS   = "ESE";
static const char *DB_SCHEMA = "elevator";

static sql::Connection *g_con = NULL;  // shared, reused across every call
static std::set<int> g_canNodes;       // can_node.can_id cache - guards the FK on can_transaction.sent_by
static long g_lastWebCmdId = 0; //Track can_transction,.id


//Returns the shared connection, opening (or re-opening) it if needed.
//Returns NULL if the database cannot be reached - callers must handle that.
static sql::Connection* db_conn(void){
	try{
		//Drop a connection that the server has since closed (e.g. MariaDB restarted)
		if(g_con != NULL && !g_con->isValid()){
			if(!g_con->reconnect()){
				delete g_con;
				g_con = NULL;
			}
		}

		if(g_con == NULL){
			sql::Driver *driver = get_driver_instance();
			g_con = driver->connect(DB_HOST, DB_USER, DB_PASS);
			g_con->setSchema(DB_SCHEMA);
		}
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] Connection failed: %s\n", e.what());
		g_con = NULL;
	}

	return g_con;
}


//Loads can_node.can_id into the cache so db_logCanTransaction() can reject unknown
//senders before the insert rather than tripping the foreign key.
static void db_loadCanNodes(void){
	sql::Connection *con = db_conn();
	if(con == NULL){
		return;
	}

	try{
		sql::Statement *stmt = con->createStatement();
		sql::ResultSet *res = stmt->executeQuery("SELECT can_id FROM can_node WHERE archived = 0");

		g_canNodes.clear();
		while(res->next()){
			g_canNodes.insert(res->getInt("can_id"));
		}

		delete res;
		delete stmt;
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] Could not read can_node: %s\n", e.what());
	}
}


int db_open(void){
	if(db_conn() == NULL){
		//Not fatal - the firmware still has to drive the CAN bus with the database down
		fprintf(stderr, "[DB] Continuing without a database connection\n");
		return -1;
	}

	db_loadCanNodes();
	db_initWebsiteCursor(); //Don't replay the history backlog 
	return 0;
}


void db_close(void){
	if(g_con != NULL){
		delete g_con;
		g_con = NULL;
	}
	g_canNodes.clear();
}


bool db_isKnownCanNode(int canId){
	//An empty cache means we never managed to read can_node; let the insert try
	//anyway rather than silently dropping every message.
	if(g_canNodes.empty()){
		return true;
	}

	return g_canNodes.find(canId) != g_canNodes.end();
}

int db_initWebsiteCursor(void){
	sql::Connection *con = db_conn();
	
	//Check connection 
	if(con == NULL){
		return -1; 
	}

	try{
		sql::Statement *stmt = con->createStatement(); 
		sql::ResultSet *res = stmt->executeQuery(
			"SELECT COALESCE(MAX(id), 0) AS maxid FROM can_transaction "
			"WHERE sent_by BETWEEN 768 AND 772");
			
		if(res->next()){
			g_lastWebCmdId = res->getInt64("maxid");
		}

		//Delete the query and statement ]
		delete res;
		delete stmt; 
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] Could not set website cursor: %s\n", e.what());
		return -1; 
	}

	return 0; 
}

//Include the get website commands 
int db_getWebsiteCommands(WebCommand *out, int maxOut){
	int count = 0; 

	sql::Connection *con = db_conn(); 
	if(con == NULL || out == NULL || maxOut <= 0){
		return -1; 
	}

	try{
		sql::PreparedStatement *pstmt = con->prepareStatement(
			"SELECT id, sent_by, data FROM can_transaction "
			"WHERE sent_by BETWEEN 768 AND 772 AND id > ? "
			"ORDER BY id ASC LIMIT ?");

		pstmt->setInt64(1, g_lastWebCmdId);
		pstmt->setInt(2, maxOut);

		//Read rge result of executing the query 
		sql::ResultSet *res = pstmt->executeQuery(); 

		long highestId = g_lastWebCmdId; 

		//Start Pointing the webcommands into the correct data format as the messages are being recieved 
		while(res->next()){
			long rowId = res->getInt64("id"); 
			int sentBy = res->getInt("sent_by");
			int data = res->getInt("data"); 

			//Check if we are at the newest row 
			if(rowId > highestId){
				highestId = rowId; 
			}

			//Dtermine the type of commamnd 
			WebCommand cmd;
			cmd.kind = WEB_CMD_NONE;
			cmd.floor = 0;
			cmd.mode = 0;
			cmd.door = 0;

			//Start Filtering per ID
			if(sentBy == 768){  //Web CC
				if(data >= 1 && data <= 3){
					cmd.kind = WEB_CMD_FLOOR_CAR;
					cmd.floor = data;
				}else if(data == WEB_DOOR_CLOSE || data == WEB_DOOR_OPEN){
					//Same node as the car floor buttons - the payload is what
					//tells the two apart (1..3 = floor, 0x08/0x09 = door)
					cmd.kind = WEB_CMD_DOOR;
					cmd.door = data;
				}
			}else if(sentBy >= 769 && sentBy <= 771 ){ //Web FC
				cmd.kind = WEB_CMD_FLOOR_REQ;
				cmd.floor = sentBy - 768; 
			}else if(sentBy == 772){ //Website Changing mode 
				if(data == WEB_MODE_ELEVATOR || data == WEB_MODE_SABBATH || data == WEB_MODE_MAINTEANCE){
					cmd.kind = WEB_CMD_MODE;
					cmd.mode = data; 
				}
			}

			//Skip anything that doesn't fit the formart 
			if(cmd.kind != WEB_CMD_NONE && count < maxOut){
				out[count] = cmd; 
				count++;
			}
		}

		//delete the result and statement 
		delete res; 
		delete pstmt; 

		g_lastWebCmdId = highestId; //Only advance after a clean read 
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] SELECT website commands Failed: %s\n", e.what());
		return -1; 
	}

	return count; 
}


//Records one CAN frame sensed by the supervisory controller.
//canId is stored as a plain decimal (0x200 -> 512) to match can_node.can_id.
int db_logCanTransaction(int canId, int data, const char *message,
                         int currentFloor, int lastFloor){
	if(!db_isKnownCanNode(canId)){
		//can_transaction.sent_by is a FK onto can_node.can_id - inserting an id that
		//is not registered would throw, so report it and move on.
		fprintf(stderr, "[DB] Skipping CAN log for unregistered node 0x%x (%d)\n", canId, canId);
		return -1;
	}

	sql::Connection *con = db_conn();
	if(con == NULL){
		return -1;
	}

	try{
		sql::PreparedStatement *pstmt = con->prepareStatement(
			"INSERT INTO can_transaction "
			"(sent_by, transceived_at, data, message, current_floor, last_floor) "
			"VALUES (?, NOW(), ?, ?, ?, ?)");

		pstmt->setInt(1, canId);
		pstmt->setInt(2, data);
		pstmt->setString(3, message != NULL ? message : "");
		pstmt->setInt(4, currentFloor);
		pstmt->setInt(5, lastFloor);
		pstmt->executeUpdate();

		delete pstmt;
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] INSERT can_transaction failed: %s\n", e.what());
		return -1;
	}

	return 0;
}


//Appends the elevator's current state - including which mode the SC is running -
//to the table the website reads.
int db_insertElevatorPosition(int currentFloor, int lastFloor,
                              int isMoving, int isClosed, const char *mode){
	sql::Connection *con = db_conn();
	if(con == NULL){
		return -1;
	}

	try{
		sql::PreparedStatement *pstmt = con->prepareStatement(
			"INSERT INTO elevator_position "
			"(current_floor, last_floor, is_moving, is_closed, mode, recorded_at) "
			"VALUES (?, ?, ?, ?, ?, NOW())");

		pstmt->setInt(1, currentFloor);
		pstmt->setInt(2, lastFloor);
		pstmt->setInt(3, isMoving ? 1 : 0);
		pstmt->setInt(4, isClosed ? 1 : 0);
		pstmt->setString(5, mode != NULL ? mode : "elevator");
		pstmt->executeUpdate();

		delete pstmt;
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] INSERT elevator_position failed: %s\n", e.what());
		return -1;
	}

	return 0;
}


//Reads the newest position row - used by the standalone receive path, which has no
//FSM to ask. Mirrors the SELECT in php/authorization/request-floor.php.
int db_getLatestPosition(int *outCurrentFloor, int *outLastFloor){
	//Floor 1 is the same fallback the website uses when the table is empty
	if(outCurrentFloor != NULL){
		*outCurrentFloor = 1;
	}
	if(outLastFloor != NULL){
		*outLastFloor = 1;
	}

	sql::Connection *con = db_conn();
	if(con == NULL){
		return -1;
	}

	try{
		sql::Statement *stmt = con->createStatement();
		sql::ResultSet *res = stmt->executeQuery(
			"SELECT current_floor, last_floor FROM elevator_position ORDER BY id DESC LIMIT 1");

		while(res->next()){
			if(outCurrentFloor != NULL){
				*outCurrentFloor = res->getInt("current_floor");
			}
			if(outLastFloor != NULL){
				*outLastFloor = res->getInt("last_floor");
			}
		}

		delete res;
		delete stmt;
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] SELECT elevator_position failed: %s\n", e.what());
		return -1;
	}

	return 0;
}
