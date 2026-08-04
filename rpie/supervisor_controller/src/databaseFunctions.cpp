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
static int g_lastFloorNum = 1;         // last good elevatorNetwork read, returned if a later read fails


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


int db_getPendingWebsiteRequest() {

	//Test the Connection to the Database 
	sql::Connection *con = db_conn();
	if(con == NULL){
		return -1;
	}


	///Prepare the SQL statementent 
	try{
		sql::Statement *stmt = con->createStatement();
		sql::ResultSet *res = stmt->executeQuery("SELECT current_floor FROM can_transaction` WHERE ");

		//Keep the last good value if the query returns no rows - returning an
		//uninitialised local (what this used to do) makes the FSM queue garbage floors.
		while(res->next()){
			g_lastFloorNum = res->getInt("current_floor");
		}

		delete res;
		delete stmt;
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] SELECT current_floor failed: %s\n", e.what());
		return -1;
	}

	return g_lastFloorNum;
}


int db_setFloorNum(int floorNum) {
	sql::Connection *con = db_conn();
	if(con == NULL){
		return -1;
	}

	try{
		sql::PreparedStatement *pstmt = con->prepareStatement("UPDATE can_transaction SET current_floor = ? WHERE nodeID = 1");
		pstmt->setInt(1, floorNum);
		pstmt->executeUpdate();
		delete pstmt;

		g_lastFloorNum = floorNum;
	}catch(sql::SQLException &e){
		fprintf(stderr, "[DB] UPDATE current_floor failed: %s\n", e.what());
		return -1;
	}

	return 0;
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
