<?php
    require_once __DIR__ . './util.php';

    // 1. Set critical headers to allow persistent even streaming
    header('Content-Type: text/event-stream');
    header('Cache-Control: no-cache');
    header('Connection: keep-alive');
    header('X-Accel-Buffering: no');

    // 2. Connect to the database
    $db = dbConnect('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');

    // 3. Trak the last ID sent to aoid repeating historical data
    $lastId = isset($_SERVER["HTTP_LAST_EVENT_ID"]) ? intval($_SERVER["HTTP_LAST_EVENT_ID"]) : 0;

    // 4. Start the server stream loop
    while (true) {
        // Check for rows newer than the last sent ID
        $statement = $db->prepare("
            select id, current_floor, last_floor, is_moving, is_closed, recorded_at
            from elevator_position
            where id > :last_id order by id asc
        ");
        $statement->execute(['last_id' => $lastId]);
        $positions = $statement->fetchAll(PDO::FETCH_ASSOC);

        foreach ($positions as $position) {
            echo "id: " . $position['id'] . "\n";
            echo "current_floor: " . $position['current_floor'] . "\n";

            $lastId = $position['id'];
        }

        if (ob_get_level() > 0) {
            ob_flush();
        }
        flush();

        sleep(10);
    }
?>