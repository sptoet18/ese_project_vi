<?php
    error_reporting(E_ALL);
    ini_set('display_errors', 1);
    ini_set('display_startup_errors', 1);

    require_once __DIR__ . '/../util.php';

    session_start();

    /*
    * Unlike set-mode.php this endpoint does NOT redirect the browser. A door
    * button that reloaded the whole page would throw away the SSE stream and
    * the door animation with it, so doorControl.js calls this with fetch()
    *
    * Field 0 is what doorControl.js switches on
    */
    header('Content-Type: text/plain; charset=utf-8');

    function doorReply($state, $message) {
        echo $state . "|" . $message;
        exit;
    }

    if (!isset($_SESSION['username'])) {
        http_response_code(401);
        doorReply('error', 'Not signed in.');
    }

    $db = dbConnect('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');

    $username = $_SESSION['username'];

    // database query for username
    $userQuery = $db->prepare('
        select id, username, hashed_password
        from user
        where username = :username
    ');
    $userQuery->execute(['username' => $username]);
    $user = $userQuery->fetch();

    if (!$user) {
        http_response_code(401);
        doorReply('error', 'Not signed in.');
    }

    /*
    * Door command codes carried in can_transaction.data by the node 768
    * "Website - Car Controller"
    * These MUST match WebDoorCode in include/databaseFunctions.h, which in turn
    * mirrors DOOR_CLOSE / DOOR_OPEN in include/pcanFunctions.h
    *
    * The same node also carries the car floor buttons as data 1, 2 and 3, so the
    * payload is what tells a door command apart from a floor request
    */
    $doorCodes = [
        'close' => 0x08,
        'open' => 0x09
    ];

    $door = '';

    if (isset($_POST['door'])) {
        $door = $_POST['door'];
    }

    if (!array_key_exists($door, $doorCodes)) {
        http_response_code(422);
        doorReply('error', 'Door must be open or close.');
    }

    /*
    * Reuse the last known position so the logged command row stays consistent
    * with the rest of can_transaction, and so we can refuse the command when the
    * elevator is in a state that must not accept it
    *
    * elevator_position is written by the firmware alone, so this is the state the
    * controller is really in, not the one somebody clicked
    */
    $positionQuery = $db->prepare('
        select current_floor, last_floor, is_moving, mode
        from elevator_position
        order by id desc
        limit 1
    ');
    $positionQuery->execute([]);
    $position = $positionQuery->fetch();

    $currentFloor = 1;
    $lastFloor = 1;
    $isMoving = 0;
    $currentMode = 'elevator';

    if ($position) {
        $currentFloor = $position['current_floor'];
        $lastFloor = $position['last_floor'];
        $isMoving = $position['is_moving'];
        $currentMode = $position['mode'];
    }

    /*
    * Sabbath honours no requests at all, and the firmware does not even poll the
    * website while it is running, so writing the row would only leave a command
    * that fires the moment somebody leaves Sabbath. Refuse it here instead
    */
    if ($currentMode === 'sabbath') {
        http_response_code(409);
        doorReply('error', 'Sabbath mode ignores door commands.');
    }

    // The door must never be touched mid travel. The firmware drops these too
    if ($isMoving) {
        http_response_code(409);
        doorReply('error', 'The car is moving. Wait until it arrives.');
    }

    /*
    * The supervisor polls can_transaction for rows sent by 768 to 772, so writing
    * the command here is what actually reaches the firmware
    */
    $commandQuery = $db->prepare('
        insert into can_transaction (
            sent_by,
            transceived_at,
            data,
            message,
            current_floor,
            last_floor
        ) values (
            768,
            now(),
            :data,
            :message,
            :current_floor,
            :last_floor
        )
    ');
    $commandQuery->execute([
        'data' => $doorCodes[$door],
        'message' => 'Website requested door ' . $door,
        'current_floor' => $currentFloor,
        'last_floor' => $lastFloor
    ]);

    doorReply('ok', ucfirst($door) . ' door command sent. Waiting for the controller.');
?>
