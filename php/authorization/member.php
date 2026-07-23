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

            $transactionQuery = $db->prepare('
                select id, sent_by, transceived_at, data, message, current_floor, last_floor
                from can_transaction
                order by id desc
            ');
            $transactionQuery->execute([]);
            $transactions = $transactionQuery->fetchAll(PDO::FETCH_ASSOC);
            
            $positionQuery = $db->prepare('
                select id, current_floor, last_floor, is_moving, is_closed, recorded_at
                from elevator_position
                order by id desc
            ');
            $positionQuery->execute([]);
            $positions = $positionQuery->fetchAll(PDO::FETCH_ASSOC);

            $elevatorPosition = getPositionImage(1);
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
                <article>
                    <div id="see"></div>
                </article>

                <article class="elevator-ui">
                    <div class="elevator-grid">
                        <div class="col">
                            <h2>Request as Floor Controller</h2>
                            <button class="elevator">Request Floor 3</button>
                            <button class="elevator">Request Floor 2</button>
                            <button class="elevator">Request Floor 1</button>
                        </div>
                        <div class="col">
                            <h2>Request as Car Controller</h2>
                            <button class="elevator">Request Floor 3</button>
                            <button class="elevator">Request Floor 2</button>
                            <button class="elevator">Request Floor 1</button>
                        </div>
                        <div class="col">
                            <h2>Elevator's Current Floor</h2>
                            <img src="<?php echo $elevatorPosition; ?>" height="340px" style="image-rendering: pixelated"/>
                        </div>
                    </div>
                </article>

                <article class="elevator-ui">
                    <div class="elevator-grid">
                        <div>
                            <h2>Elevator Mode</h2>
                            <button class="elevator">Start Elevator Mode</button>
                        </div>
                        <div>
                            <h2>Sabbath Mode</h2>
                            <button class="elevator">Start Sabbath Mode</button>
                        </div>
                        <div>
                            <h2>Maintenance Mode</h2>
                            <button class="elevator">Start Maintenance Mode</button>
                        </div>
                    </div>
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
    <script src="/js/components/top-bar.js"></script>
    <script src="/js/components/copyright.js" defer></script>
</body>
</html>