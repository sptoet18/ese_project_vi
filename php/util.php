<?php
    // --------- Database ---------
    function dbConnect(string $path, string $user, string $password) {
        $db = new PDO($path, $user, $password);
        $db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);

        return $db;
    }

    // Create
    function insert($path, $user, $password, $current_date, $current_time, $status, $currentFloor, $requestedFloor, $otherInfo) {
        $db = dbConnect($path, $user, $password);
        $query = 'INSERT INTO elevatorNetwork(date, time, status, currentFloor, requestedFloor, otherInfo) VALUES
        (:date, :time, :status, :currentFloor, :requestedFloor, :otherInfo)';
        $params = [
            'date' => $current_date['CURRENT_DATE()'],
            'time' => $current_time['CURRENT_TIME()'],
            'status' => $status, 
            'currentFloor' => $currentFloor,
            'requestedFloor' => $requestedFloor, 
            'otherInfo' => $otherInfo
        ];
        $statement = $db->prepare($query);
        $result = $statement->execute($params); 
    }

    //Mannually insert into user tabele 
    function insert_usr($path, $user, $password, $username, $password_db, $firstname, $lastname, $role) {
        $db = dbConnect($path, $user, $password);
        $query = 'INSERT INTO user(username, hashed_password, firstname, lastname, role) VALUES
        (:username, :password_db, :firstname, :lastname, :role)';
        $params = [
            'username' => $username,
            'password_db' => password_hash($password_db, PASSWORD_DEFAULT), 
            'firstname' => $firstname,
            'lastname' => $lastname, 
            'role' => $role
        ];
        $statement = $db->prepare($query);
        $result = $statement->execute($params);
        
        if($result == false){
            $error = $db->errorInfo();
            echo "ERROR: " . $error[2];
        } else {
            //var_dump($result);
        }
        
        
    }


    // Read
    function showtable(string $path, string $user, string $password, $tablename) {
        echo "<h3>Content of ElevatorNetwork table</h3>";
        $db = dbConnect($path, $user, $password); 
        $query = "SELECT * FROM $tablename GROUP BY nodeID ORDER BY nodeID";  // Note: Risk of SQL injection
        $rows = $db->query($query); 
        echo "DATE|TIME|NODEID|STATUS|CURRENTFLOOR|REQUESTED FLOOR|OTHERINFO <br>";
        foreach ($rows as $row) {
            echo $row['date'] . " | " . $row['time'] . " | " . $row['nodeID'] . " | " . $row['status'] . " | " 
                . $row['currentFloor'] . " | " . $row['requestedFloor'] . " | " . $row['otherInfo'] . "<br>";
        }
    }

    // Update
    function update(string $path, string $user, string $password, string $tablename, int $node_ID, int $new_status, int $new_currentFloor, 
                    int $new_requestedFloor, string $new_otherInfo) : void {
        $db = dbConnect($path, $user, $password);
        $query = 'UPDATE ' . $tablename . ' SET status = :stat, currentFloor = :curFloor, requestedFloor = :rqFloor, otherInfo = :oInfo
                WHERE nodeID = :id' ;    // Note: Risks of SQL injection
        $statement = $db->prepare($query); 
        $statement->bindValue('stat', $new_status); 
        $statement->bindValue('curFloor', $new_currentFloor);
        $statement->bindValue('rqFloor', $new_requestedFloor);
        $statement->bindValue('oInfo', $new_otherInfo);
        $statement->bindValue('id', $node_ID); 
        $statement->execute();                      // Execute prepared statement
    }
    // Delete
    function delete(string $path, string $user, string $password, string $tablename, int $node_ID) : void {
        $db = dbConnect($path, $user, $password);
        $query = 'DELETE FROM ' . $tablename . ' WHERE nodeID = :id' ;    // Note: Risks of SQL injection
        $statement = $db->prepare($query); 
        $statement->bindValue('id', $node_ID); 
        $statement->execute();                      // Execute prepared statement
    }

    // --------- Passwords ---------

    // Verify Password
    function isCorrectPassword(string $password, string $hashedPassword) : bool {
        if (password_hash($password, PASSWORD_DEFAULT) === $hashedPassword) {
            return true;
        } else {
            return false;
        }
    }

    // --------- Elevator Position ---------

    function getPositionImage(int $position) {
        switch ($position) {
            case 1:
                return "/assets/images/floor-indicators/floor1.png";
            case 2:
                return "/assets/images/floor-indicators/floor2.png";
            case 3:
                return "/assets/images/floor-indicators/floor3.png";
        }
    }

?>