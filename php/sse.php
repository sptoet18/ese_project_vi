<?php
    require_once __DIR__ . '/util.php';

    set_time_limit(0);
    ignore_user_abort(true);

    header('Content-Type: text/event-stream');
    header('Cache-Control: no-cache');
    header('Connection: keep-alive');
    header('X-Accel-Buffering: no');

    $db = dbConnect('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');

    // 3. Trak the last ID sent to aoid repeating historical data
    if (isset($_SERVER["HTTP_LAST_EVENT_ID"])) {
        // A reconnect. Pick up exactly where the dropped stream left off
        $lastId = intval($_SERVER["HTTP_LAST_EVENT_ID"]);
    } else {
        /*
        * A FRESH connection carries no Last-Event-ID, so starting at 0 used to
        * replay the whole elevator_position table - hundreds of rows in one
        * burst. The door diagram animated through every state the elevator has
        * ever been in before settling, and the oldest rows predate the mode
        * column being filled in, so the mode chip blanked on the way past
        *
        * Seed one BELOW the newest id instead. "id > max - 1" matches the newest
        * row and nothing else whatever the ids are, so the client still gets a
        * state to sync to immediately rather than waiting for the next change
        */
        $newestQuery = $db->prepare('
            select coalesce(max(id), 0) - 1 as start_id
            from elevator_position
        ');
        $newestQuery->execute([]);
        $newest = $newestQuery->fetch();

        $lastId = intval($newest['start_id']);
    }

    // 4. Start the server stream loop
    while (true) {
        if (connection_aborted()) {
            break;
        }
        // Check for rows newer than the last sent ID
        $statement = $db->prepare("
            select id, current_floor, last_floor, is_moving, is_closed, mode, recorded_at
            from elevator_position
            where id > :last_id order by id asc
        ");
        $statement->execute(['last_id' => $lastId]);
        $positions = $statement->fetchAll(PDO::FETCH_ASSOC);

        foreach ($positions as $position) {
            /*
            * One row goes out as five pipe separated fields, in this order:
            *
            *   current_floor | last_floor | is_moving | is_closed | mode
            *
            * event-source.js splits on the same character. The mode is the
            * only text field and the firmware only ever writes elevator,
            * sabbath or maintenance, so no field can contain a pipe itself
            */
            $payload = $position['current_floor'] . "|"
                . $position['last_floor'] . "|"
                . $position['is_moving'] . "|"
                . $position['is_closed'] . "|"
                . $position['mode'];

            echo "id: " . $position['id'] . "\n";
            echo "data: " . $payload . "\n\n";

            $lastId = $position['id'];
        }

        // Keeps the stream alive when the elevator sits still
        echo ": heartbeat\n\n";

        if (ob_get_level() > 0) {
            ob_flush();
        }
        flush();

        sleep(1);
    }
?>
