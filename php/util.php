<?php
    // --------- Database ---------
    function dbConnect(string $path, string $user, string $password) {
        $db = new PDO($path, $user, $password);
        $db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);

        return $db;
    }

    function connect() : PDO {
        try {
            $db = new PDO('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');
            $db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
        } catch (PDOException $e) {
            die("Connection failed: " . $e->getMessage());
        }

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

    //Manually insert into user table
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

    // --------- Static assets ---------

    /*
    * Appends a stylesheet or script's last modified time to its URL:
    *
    *   /css/style.css  ->  /css/style.css?v=1786100542
    *
    * Apache serves these files with an ETag and a Last-Modified but NO
    * Cache-Control and no Expires, and mod_expires/mod_headers are not enabled.
    * With no explicit directive the browser falls back to heuristic freshness
    * and reuses its cached copy without asking the server whether it changed
    *
    * The PHP pages themselves are always refetched, because session_start()
    * sends "Cache-Control: no-store, no-cache, must-revalidate" - which is why
    * edits to the markup appeared straight away while edits to the stylesheet
    * did not, and why a hard reload was the only way to see them
    *
    * The stamp changes only when the file does, so the browser refetches once
    * after an edit and caches normally the rest of the time
    */
    function assetUrl(string $webPath) : string {
        //util.php lives in <docroot>/php, so its parent IS the document root
        $filePath = dirname(__DIR__) . $webPath;

        $stamp = @filemtime($filePath);

        if ($stamp === false) {
            return $webPath; //Unknown file - hand back the URL untouched
        }

        return $webPath . '?v=' . $stamp;
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