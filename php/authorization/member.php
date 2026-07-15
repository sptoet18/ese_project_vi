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
                <article class="elevator-ui">
                    <div class="container">
                        <div class="row">
                            <div class="col">
                                <div class="controls">
                                    <button class="elevator">Request Floor 3</button>
                                    <button class="elevator">Request Floor 2</button>
                                    <button class="elevator">Request Floor 1</button>
                                </div>
                            </div>
                            <div class="col">
                                <div class="indicators">
                                    <div class="indicator">Floor 3</div>
                                    <div class="indicator">Floor 2</div>
                                    <div class="indicator">Floor 1</div>
                                </div>
                            </div>
                        </div>
                        <div class="indicator">Moving</div>
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
    <script>
        document.getElementById("date").textContent = new Date().toLocaleDateString();
        document.getElementById("time").textContent = new Date().toLocaleTimeString();
    </script>
</body>
</html>