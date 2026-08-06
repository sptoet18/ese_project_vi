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
            * Both printouts are capped at the 100 newest rows. Unbounded these
            * Row [0] is still the newest either way, so the current floor, mode
            * and door state read off it below are unaffected
            */
            $transactionQuery = $db->prepare('
                select *
                from can_transaction
                order by id desc
                limit 100
            ');
            $transactionQuery->execute([]);
            $transactions = $transactionQuery->fetchAll(PDO::FETCH_ASSOC);

            $positionQuery = $db->prepare('
                select *
                from elevator_position
                order by id desc
                limit 100
            ');
            $positionQuery->execute([]);
            $positions = $positionQuery->fetchAll(PDO::FETCH_ASSOC);

            /*
            * Get the most recent known elevator position and mode
            * If the table is empty, fall back to floor 1 in elevator mode
            *
            * The mode is the one the CONTROLLER is in, not the one somebody
            * clicked. Only the firmware writes elevator_position, so this is
            * honest
            */
            if ($positions) {
                $currentFloor = $positions[0]["current_floor"];
                $currentMode = $positions[0]["mode"];
                $isMoving = $positions[0]["is_moving"];
                $isClosed = $positions[0]["is_closed"];
            } else {
                $currentFloor = 1;
                $currentMode = "elevator";
                $isMoving = 0;
                $isClosed = 1;
            }

            $elevatorPosition = getPositionImage($currentFloor);

            /*
            *send the door diagram from the same row, so it renders in the right
            * position on load instead of waiting a second for the first SSE tick.
            * event-source.js writes exactly these three values afterwards
            */
            if ($isMoving) {
                $currentDoor = "moving";
            } else if ($isClosed) {
                $currentDoor = "closed";
            } else {
                $currentDoor = "open";
            }

            /*
            * set-mode.php leaves a one shot message in the session and sends
            * the browser back here. Read it once and clear it, so a refresh
            * does not keep showing a stale confirmation
            */
            $modeStatus = "";

            if (isset($_SESSION['modeStatus'])) {
                $modeStatus = $_SESSION['modeStatus'];
            }

            unset($_SESSION['modeStatus']);
        } else {
            echo "<script>location.href = \"/html/authorization/login.html\"</script>";
        }
    } else {
        echo "<script>location.href = \"/html/authorization/login.html\"</script>";
    }
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="Elevator Controller">
    <meta name="robots" content="noindex nofollow"/>
    <meta http-equiv="author" content="Emiliano Perez Pellicer, Sean Toet, Besart Kalezic">
    <meta http-equiv="pragma" content="no-cache"/> 
    <title>Elevator Controller</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">
    <!-- assetUrl() stamps each file with its mtime so an edited stylesheet is
         refetched instead of being served from the browser's cache -->
    <link href="<?= assetUrl('/css/theme.css') ?>" rel="stylesheet"/>
    <link href="<?= assetUrl('/css/components/top-bar-style.css') ?>" rel="stylesheet"/>
    <link href="<?= assetUrl('/css/style.css') ?>" rel="stylesheet"/>
</head>
<body class="d-flex flex-column min-vh-100">
    <header>
        <div id="topbar"></div>
    </header>

    <main class="flex-grow-1">
        <div class="container">
            <section class="title">
                <h1>Elevator Controls</h1>
                <h2>Electronic System Engineering</h2>
            </section>

            <section class="body">
                <article class="elevator-ui">
                    <div class="elevator-grid">
                        <!-- Floor-controller requests -->
                        <div class="col">
                            <h2>Request as Floor Controller</h2>
                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="floor_controller"
                                data-floor="3"
                                <?php if ($currentMode === "sabbath") echo "disabled"; ?>
                            >
                                Request Floor 3
                            </button>
                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="floor_controller"
                                data-floor="2"
                                <?php if ($currentMode === "sabbath") echo "disabled"; ?>
                            >
                                Request Floor 2
                            </button>
                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="floor_controller"
                                data-floor="1"
                                <?php if ($currentMode === "sabbath") echo "disabled"; ?>
                            >
                                Request Floor 1
                            </button>
                        </div>


                        <div class="col">
                            <h2>Request as Car Controller</h2>
                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="car_controller"
                                data-floor="3"
                                <?php if ($currentMode === "sabbath") echo "disabled"; ?>
                            >
                                Request Floor 3
                            </button>

                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="car_controller"
                                data-floor="2"
                                <?php if ($currentMode === "sabbath") echo "disabled"; ?>
                            >
                                Request Floor 2
                            </button>

                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="car_controller"
                                data-floor="1"
                                <?php if ($currentMode === "sabbath") echo "disabled"; ?>
                            >
                                Request Floor 1
                            </button>
                        </div>
                        
                        <div class="col">
                            <h2>Elevator's Current Floor</h2>
                            <img id="floor-indicator" src="<?php echo $elevatorPosition; ?>" height="340px" style="image-rendering: pixelated"/>

                            <!--
                                Sits INSIDE this column rather than after the grid.
                                Loose at the bottom of the article it rendered hard
                                against the left margin, reading as a stray message
                                instead of the caption for the indicator above it
                            -->
                            <p
                                id="state-chip"
                                role="status"
                                aria-live="polite"
                            >At floor <?= htmlspecialchars($currentFloor) ?></p>
                        </div>
                    </div>

                    <p
                        id="request-status"
                        role="status"
                        aria-live="polite"
                        data-state=""
                    ></p>
                </article>

                <!--
                    Door control. Its own article rather than a fourth cell in the
                    3 column elevator-grid above, which is what keeps it centred.
                -->
                <article class="elevator-ui door-ui">
                    <h2>Elevator Door</h2>

                    <div id="door-diagram" data-door="<?= htmlspecialchars($currentDoor) ?>" role="img"
                         aria-label="Elevator door position">
                        <div class="door-cab"></div>
                        <div class="door-panel left"></div>
                        <div class="door-panel right"></div>
                    </div>

                    <p id="door-state-label" data-door="<?= htmlspecialchars($currentDoor) ?>">
                        <?php
                            if ($currentDoor === "moving") {
                                echo "Car moving";
                            } else if ($currentDoor === "closed") {
                                echo "Door closed";
                            } else {
                                echo "Door open";
                            }
                        ?>
                    </p>

                    <div class="door-controls">
                        <button
                            type="button"
                            class="elevator door-request"
                            data-door="open"
                            <?php if ($currentMode === "sabbath" || $isMoving) echo "disabled"; ?>
                        >
                            Open Door
                        </button>

                        <button
                            type="button"
                            class="elevator door-request"
                            data-door="close"
                            <?php if ($currentMode === "sabbath" || $isMoving) echo "disabled"; ?>
                        >
                            Close Door
                        </button>
                    </div>

                    <p
                        id="door-status"
                        role="status"
                        aria-live="polite"
                        data-state=""
                    ></p>
                </article>

                <article class="elevator-ui">
                    <!--
                        Plain form posts. set-mode.php writes the mode command
                        to can_transaction and sends the browser straight back
                        here, so no scripting sits between the button and the
                        database
                    -->
                    <form method="post" action="/php/modes/set-mode.php">
                        <div class="elevator-grid">
                            <div>
                                <h2>Elevator Mode</h2>
                                <button type="submit" name="mode" value="elevator" class="elevator">Start Elevator Mode</button>
                            </div>
                            <div>
                                <h2>Sabbath Mode</h2>
                                <button type="submit" name="mode" value="sabbath" class="elevator">Start Sabbath Mode</button>
                            </div>
                            <div>
                                <h2>Maintenance Mode</h2>
                                <button type="submit" name="mode" value="maintenance" class="elevator">Start Maintenance Mode</button>
                            </div>
                            <!--
                                Spans the full row. As a plain 4th cell in a 3
                                column grid it wrapped onto its own line but kept
                                column 1's width, so the readout sat off to the
                                left under "Elevator Mode" as if it belonged to it
                            -->
                            <div class="mode-active">
                                <h2>Active Mode</h2>
                                <p id="mode-chip" data-mode="<?= htmlspecialchars($currentMode) ?>"><?= htmlspecialchars($currentMode) ?></p>
                                <p id="mode-status" role="status" aria-live="polite"><?= htmlspecialchars($modeStatus) ?></p>
                            </div>
                        </div>
                    </form>
                </article>

                <?php if (true): ?>
                    <article>
                        <div style="margin-top: 36px;"></div>

                        <h1>Maintenance Printout</h1>

                        <div class="maintenance-grid">
                            <div>
                                <h2>Can Transactions</h2>
                                <div class="maintenance-card">
                                    <table class="console">
                                        <thead>
                                            <tr>
                                                <th>Sender</th>
                                                <th>Time</th>
                                                <th>Data</th>
                                                <th>Floor</th>
                                                <th>Last Floor</th>
                                            </tr>
                                        </thead>

                                        <tbody>
                                            <?php if (count($transactions)): ?>
                                                <?php foreach ($transactions as $row): ?>
                                                    <tr>
                                                        <td><?= htmlspecialchars($row['sent_by']) ?></td>
                                                        <td><?= htmlspecialchars($row['transceived_at']) ?></td>
                                                        <td><?= htmlspecialchars($row['data']) ?></td>
                                                        <td><?= htmlspecialchars($row['current_floor']) ?></td>
                                                        <td><?= htmlspecialchars($row['last_floor']) ?></td>
                                                    </tr>
                                                <?php endforeach; ?>
                                            <?php else: ?>
                                                <tr>
                                                    <td colspan="5">No transactions found.</td>
                                                </tr>
                                            <?php endif; ?>
                                        </tbody>
                                    </table>
                                </div>
                            </div>
                            <div>
                                <h2>Elevator Position</h2>
                                <div class="maintenance-card">
                                    <table class="console">
                                        <thead>
                                            <tr>
                                                <th>Current Floor</th>
                                                <th>Time</th>
                                                <th>Last Floor</th>
                                                <th>Moving</th>
                                                <th>Door Closed</th>
                                                <th>Mode</th>
                                            </tr>
                                        </thead>

                                        <tbody>
                                            <?php if (count($positions) > 0): ?>
                                                <?php foreach ($positions as $row): ?>
                                                    <tr>
                                                        <td><?= htmlspecialchars($row['current_floor']) ?></td>
                                                        <td><?= htmlspecialchars($row['recorded_at']) ?></td>
                                                        <td><?= htmlspecialchars($row['last_floor']) ?></td>
                                                        <td><?= htmlspecialchars($row['is_moving']) ?></td>
                                                        <td><?= htmlspecialchars($row['is_closed']) ?></td>
                                                        <td><?= htmlspecialchars($row['mode']) ?></td>
                                                    </tr>
                                                <?php endforeach; ?>
                                            <?php else: ?>
                                                <tr>
                                                    <td colspan="6">No transactions found.</td>
                                                </tr>
                                            <?php endif; ?>
                                        </tbody>
                                    </table>             
                                </div>
                            </div>
                        </div>
                    </article> 
                <?php endif; ?>
            </section>
        </div>
    </main>
    
    <footer>
        <div class="container">
            <copyright-text name="Emiliano Perez Pellicer, Besart Kalezic, Sean Toet"></copyright-text>
        </div>
    </footer>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js" integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI" crossorigin="anonymous"></script>
    <script src="<?= assetUrl('/js/components/top-bar.js') ?>"></script>
    <script src="<?= assetUrl('/js/components/copyright.js') ?>" defer></script>
    <script src="<?= assetUrl('/js/elevatorControl.js') ?>" defer></script>
    <script src="<?= assetUrl('/js/doorControl.js') ?>" defer></script>
    <script src="<?= assetUrl('/js/components/event-source.js') ?>" defer></script>
</body>
</html>