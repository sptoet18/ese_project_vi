<?php
    error_reporting(E_ALL);
    ini_set('display_errors', 1);
    ini_set('display_startup_errors', 1);

    require_once __DIR__ . '/../util.php';

    session_start();

    $db = dbConnect('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');

    if (isset($_SESSION['username'])) {

        $username = $_SESSION['username'];

        // database query for username
        $userQuery = $db->prepare('
            select id, username, hashed_password
            from user
            where username = :username
        ');
        $userQuery->execute(['username' => $username]);
        $user = $userQuery->fetch();

        if ($user) {

            /*
            * Mode command codes carried in can_transaction.data by the
            * node 772 "Website - Supervisor"
            * These MUST match WebModeCode in include/databaseFunctions.h
            */
            $modeCodes = [
                'elevator' => 0x10,
                'sabbath' => 0x11,
                'maintenance' => 0x12
            ];

            // The mode comes from the plain HTML form on member.php
            $mode = '';

            if (isset($_POST['mode'])) {
                $mode = $_POST['mode'];
            }

            if (array_key_exists($mode, $modeCodes)) {

                /*
                * Reuse the last known position so the logged command row
                * stays consistent with the rest of can_transaction
                * If the table is empty, fall back to floor 1
                */
                $positionQuery = $db->prepare('
                    select current_floor, last_floor
                    from elevator_position
                    order by id desc
                    limit 1
                ');
                $positionQuery->execute([]);
                $position = $positionQuery->fetch();

                $currentFloor = 1;
                $lastFloor = 1;

                if ($position) {
                    $currentFloor = $position['current_floor'];
                    $lastFloor = $position['last_floor'];
                }

                /*
                * The supervisor polls can_transaction for rows sent by 768 to
                * 772, so writing the command here is what actually reaches the
                * firmware. elevator_position is written by the controller
                * alone, which is what makes the mode shown on member.php the
                * mode the elevator is really in instead of the one we clicked
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
                        772,
                        now(),
                        :data,
                        :message,
                        :current_floor,
                        :last_floor
                    )
                ');
                $commandQuery->execute([
                    'data' => $modeCodes[$mode],
                    'message' => 'Website requested ' . $mode . ' mode',
                    'current_floor' => $currentFloor,
                    'last_floor' => $lastFloor
                ]);

                $_SESSION['modeStatus'] = ucfirst($mode)
                    . ' mode requested. Waiting for the controller to confirm.';
            } else {
                $_SESSION['modeStatus'] =
                    'Mode must be elevator, sabbath or maintenance.';
            }

            // Hand the user back to the control page either way
            echo "<script>location.href = \"/php/authorization/member.php\"</script>";
        } else {
            echo "<script>location.href = \"/html/authorization/login.html\"</script>";
        }
    } else {
        echo "<script>location.href = \"/html/authorization/login.html\"</script>";
    }
?>
