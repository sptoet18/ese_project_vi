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
                select *
                from can_transaction
                order by id desc
                limit 25
            ');
        $transactionQuery->execute([]);
        $transactions = $transactionQuery->fetchAll(PDO::FETCH_ASSOC);

        $positionQuery = $db->prepare('
                select *
                from elevator_position
                order by id desc
                limit 25
            ');
        $positionQuery->execute([]);
        $positions = $positionQuery->fetchAll(PDO::FETCH_ASSOC);

        /*
        * Get the most recent known elevator position
        * If the table is empty, return the default image for floor 1
        */
        $elevatorPosition = getPositionImage($positions ? $positions[0]["current_floor"] : 1);
    } else {
        header('Location: /html/authorization/login.html');
    }
} else {
    header('Location: /html/authorization/login.html');
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="Database Controls">
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
            <h1>Database Controls</h1>
            <h2>Electronic System Engineering</h2>
        </section>

        <section class="body">
            <article>
                <div style="margin-top: 36px;"></div>
                    <h2>Can Transactions</h2>
                    <div class="maintenance-card">
                        <table class="console">
                            <thead>
                            <tr>
                                <th>ID</th>
                                <th>Sender</th>
                                <th>Time</th>
                                <th>Data</th>
                                <th>Floor</th>
                                <th>Last Floor</th>
                                <th>Toggle Editing</th>
                                <th>Save Edit</th>
                            </tr>
                            </thead>

                            <tbody>
                                <?php if (count($transactions)): ?>
                                    <?php foreach ($transactions as $row): ?>
                                        <tr>
                                            <td><p class="id" data-field="id"><?= htmlspecialchars($row['id']) ?> </p></td>
                                            <td><p class="editable" data-field="sent_by"> <?= htmlspecialchars($row['sent_by']) ?> </p></td>
                                            <td><p data-field="transceived_at"><?= htmlspecialchars($row['transceived_at']) ?> </p></td>
                                            <td><p class="editable" data-field="data"><?= htmlspecialchars($row['data']) ?> </p></td>
                                            <td><p class="editable" data-field="current_floor"><?= htmlspecialchars($row['current_floor']) ?> </p></td>
                                            <td><p class="editable" data-field="last_floor"><?= htmlspecialchars($row['last_floor']) ?> </p></td>
                                            <td><input id="editing" class="editing-toggle" type="checkbox"></td>
                                            <td><button id="save" class="save-button" style="display: none">Save</button></td>
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
<script src="/js/database-controls.js"></script>
</body>
</html>