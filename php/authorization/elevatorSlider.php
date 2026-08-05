<?php

declare(strict_types=1);

require_once __DIR__ . '/../util.php';

session_start();

if (!isset($_SESSION['username'])) {
    header('Location: ../../html/authorization/login.html');
    exit();
}

$db = dbConnect(
    'mysql:host=127.0.0.1;dbname=elevator;charset=utf8mb4',
    'Emiliano',
    'ESE'
);

/*
 * Confirm the logged-in user exists.
 */
$userStatement = $db->prepare(
    '
    SELECT id
    FROM user
    WHERE username = :username
    LIMIT 1
    '
);

$userStatement->execute([
    'username' => $_SESSION['username']
]);

$user = $userStatement->fetch(PDO::FETCH_ASSOC);

if (!$user) {
    session_destroy();

    header('Location: ../../html/authorization/login.html');
    exit();
}

/*
 * Read the newest known elevator position.
 */
$positionStatement = $db->query(
    '
    SELECT current_floor, last_floor, mode
    FROM elevator_position
    ORDER BY recorded_at DESC, id DESC
    LIMIT 1
    '
);

$position = $positionStatement->fetch(PDO::FETCH_ASSOC);

$currentFloor = $position
    ? (int) $position['current_floor']
    : 1;

$currentMode = $position
    ? (string) $position['mode']
    : 'elevator';

$sliderDisabled = $currentMode === 'sabbath';

?>

<!DOCTYPE html>

<html lang="en">
<head>
    <meta charset="UTF-8">

    <meta
        name="viewport"
        content="width=device-width, initial-scale=1.0"
    >

    <meta
        name="description"
        content="Interactive elevator floor slider"
    >

    <meta
        name="robots"
        content="noindex nofollow"
    >

    <meta
        http-equiv="author"
        content="Besart Kalezic"
    >

    <title>Interactive Elevator Slider</title>

    <link
        href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css"
        rel="stylesheet"
        integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB"
        crossorigin="anonymous"
    >

    <link
        href="../../css/theme.css"
        rel="stylesheet"
    >

    <link
        href="../../css/style.css"
        rel="stylesheet"
    >

    <link
        href="../../css/elevator-slider.css"
        rel="stylesheet"
    >
</head>

<body class="d-flex flex-column min-vh-100">

<header>
    <div id="topbar"></div>
</header>

<main class="flex-grow-1">

    <div class="container slider-page">

        <section class="title">
            <h1>Interactive Elevator Slider</h1>

            <h2>
                Select a destination floor
            </h2>
        </section>

        <section class="slider-card">

            <div class="elevator-shaft">

                <div class="floor-row floor-three">
                    <span class="floor-number">3</span>
                    <span class="floor-name">Third Floor</span>
                </div>

                <div class="floor-row floor-two">
                    <span class="floor-number">2</span>
                    <span class="floor-name">Second Floor</span>
                </div>

                <div class="floor-row floor-one">
                    <span class="floor-number">1</span>
                    <span class="floor-name">First Floor</span>
                </div>

                <div class="slider-track-container">

                    <input
                        type="range"
                        id="floor-slider"
                        min="1"
                        max="3"
                        step="1"
                        value="<?= htmlspecialchars((string) $currentFloor) ?>"
                        aria-label="Select elevator destination floor"
                        <?= $sliderDisabled ? 'disabled' : '' ?>
                    >

                </div>

            </div>

            <div class="slider-information">

                <div class="floor-information-box">

                    <span class="information-label">
                        Current Floor
                    </span>

                    <strong id="current-floor">
                        <?= htmlspecialchars((string) $currentFloor) ?>
                    </strong>

                </div>

                <div class="floor-information-box">

                    <span class="information-label">
                        Selected Floor
                    </span>

                    <strong id="selected-floor">
                        <?= htmlspecialchars((string) $currentFloor) ?>
                    </strong>

                </div>

                <div class="floor-information-box">

                    <span class="information-label">
                        Current Mode
                    </span>

                    <strong>
                        <?= htmlspecialchars(ucfirst($currentMode)) ?>
                    </strong>

                </div>

                <button
                    type="button"
                    id="send-floor-request"
                    class="elevator slider-request-button"
                    <?= $sliderDisabled ? 'disabled' : '' ?>
                >
                    Send Floor Request
                </button>

                <?php if ($sliderDisabled): ?>

                    <p class="slider-warning">
                        Floor requests are unavailable while Sabbath mode is active.
                    </p>

                <?php endif; ?>

                <p
                    id="slider-status"
                    class="slider-status"
                    role="status"
                    aria-live="polite"
                ></p>

                <a
                    href="member.php"
                    class="return-link"
                >
                    Return to Elevator Controls
                </a>

            </div>

        </section>

    </div>

</main>

<footer>
    <div class="container">
        <copyright-text
            name="Emiliano Perez Pellicer, Besart Kalezic, Sean Toet"
        ></copyright-text>
    </div>
</footer>

<script
    src="../../js/components/top-bar.js"
></script>

<script
    src="../../js/components/copyright.js"
    defer
></script>

<script
    src="../../js/elevatorSlider.js"
    defer
></script>

<script
    src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js"
    integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI"
    crossorigin="anonymous"
></script>

</body>
</html>