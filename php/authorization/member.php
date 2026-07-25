<?php
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
            // Add query to get the elevator's current location

            $elevatorPosition = getPositionImage(1);
        } else {
            echo "<script>location.href = \"/html/authorization/login.html\"</script>";
        }
    } else {
        echo "<script>location.href = \"/html/authorization/login.html\"</script>";
    }

/*
 * Get the most recent known elevator position 
 * If the table is empty, return the default image for floor 1
 * 
 */
  $positionQuery = $db->query(
        '
        SELECT current_floor, last_floor, is_moving, is_closed
        FROM elevator_position
        ORDER BY recorded_at DESC, id DESC
        LIMIT 1
        '
    );
    $position = $positionQuery->fetch();
    $currentFloor = $position ? $position['current_floor'] : 1;


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
    <link href="/css/theme.css" rel="stylesheet"/>
    <link href="/css/components/top-bar-style.css" rel="stylesheet"/>
    <link href="/css/style.css" rel="stylesheet"/>
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
                        <div>
                            <!-- <h2>Request as Floor Controller</h2>
                            <button class="elevator">Request Floor 3</button>
                            <button class="elevator">Request Floor 2</button>
                            <button class="elevator">Request Floor 1</button> -->
                            <h2>Request as Floor Controller</h2>
                         <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="floor_controller"
                                data-floor="3"
                            >
                                Request Floor 3
                            </button>
                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="floor_controller"
                                data-floor="2"
                            >
                                Request Floor 2
                            </button>
                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="floor_controller"
                                data-floor="1"
                            >
                                Request Floor 1
                            </button>
                        </div>


                        <div>
                            <!-- <h2>Request as Car Controller</h2>
                            <button class="elevator">Request Floor 3</button>
                            <button class="elevator">Request Floor 2</button>
                            <button class="elevator">Request Floor 1</button> -->
                         <h2>Request as Car Controller</h2>

                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="car_controller"
                                data-floor="3"
                            >
                                Request Floor 3
                            </button>

                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="car_controller"
                                data-floor="2"
                            >
                                Request Floor 2
                            </button>

                            <button
                                type="button"
                                class="elevator floor-request"
                                data-controller="car_controller"
                                data-floor="1"
                            >
                                Request Floor 1
                            </button>
                        
                        
                        </div>
                        <div>
                            <h2>Elevator's Current Floor</h2>
                            <img src="<?php echo $elevatorPosition; ?>" height="340px" style="image-rendering: pixelated"/>
                        </div>
                    </div>
                </article>
            </section>
        </div>
    </main>
    
    <footer>
        <div class="container">
            <copyright-text name="Emiliano Perez Pellicer, Besart Kalezic, Sean Toet"></copyright-text>
        </div>
    </footer>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js" integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI" crossorigin="anonymous"></script>
    <script src="/js/components/top-bar.js"></script>
    <script src="/js/components/copyright.js" defer></script>
    <script src="../../js/components/elevatorControl.js" defer></script>

    <script>
        document.getElementById("date").textContent = new Date().toLocaleDateString();
        document.getElementById("time").textContent = new Date().toLocaleTimeString();
    </script>
</body>
</html>