<?php

declare(strict_types=1);

require_once __DIR__ . '/../util.php';

session_start();

header('Content-Type: application/json; charset=utf-8');

function sendJsonResponse(
    int $statusCode,
    array $response
): never {
    http_response_code($statusCode);
    echo json_encode($response);
    exit;
}

/*
 * Only logged-in users may create requests.
 */
if (!isset($_SESSION['username'])) {
    sendJsonResponse(401, [
        'success' => false,
        'message' => 'You must be logged in.'
    ]);
}

/*
 * This endpoint only accepts POST requests.
 */
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    sendJsonResponse(405, [
        'success' => false,
        'message' => 'Only POST requests are allowed.'
    ]);
}

try {
    $requestBody = file_get_contents('php://input');

    if ($requestBody === false || $requestBody === '') {
        throw new InvalidArgumentException(
            'No request data was received.'
        );
    }

    $requestData = json_decode(
        $requestBody,
        true,
        512,
        JSON_THROW_ON_ERROR
    );

    $requestedFloor = filter_var(
        $requestData['requested_floor'] ?? null,
        FILTER_VALIDATE_INT
    );

    $controllerType =
        $requestData['controller_type'] ?? null;

    /*
     * Only the three valid floors are accepted.
     */
    if (
        $requestedFloor === false ||
        !in_array($requestedFloor, [1, 2, 3], true)
    ) {
        throw new InvalidArgumentException(
            'Requested floor must be 1, 2, or 3.'
        );
    }

    /*
     * Prevent users from submitting arbitrary controller names.
     */
    $controllerLabels = [
        'floor_controller' => 'Floor controller',
        'car_controller' => 'Car controller'
    ];

    if (!array_key_exists($controllerType, $controllerLabels)) {
        throw new InvalidArgumentException(
            'Invalid controller type.'
        );
    }

    /*
     * CAN node IDs from the can_node table:
     *
     * 512 = Car Controller
     * 513 = Floor 1 Controller
     * 514 = Floor 2 Controller
     * 515 = Floor 3 Controller
     */
    if ($controllerType === 'car_controller') {
        $sentByCanId = 512;
    } else {
        $floorControllerCanIds = [
            1 => 513,
            2 => 514,
            3 => 515
        ];

        $sentByCanId =
            $floorControllerCanIds[$requestedFloor];
    }



    $db = dbConnect(
        'mysql:host=127.0.0.1;dbname=elevator;charset=utf8mb4',
        'Emiliano',
        'ESE'
    );

    /*
     * Confirm that the session still belongs to a real user.
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
    sendJsonResponse(401, [
        'success' => false,
        'message' => 'The logged-in user could not be found.'
    ]);
}

    /*
     * Read the latest actual elevator position.
     */
    $positionStatement = $db->query(
        '
        SELECT current_floor, last_floor
        FROM elevator_position
        ORDER BY recorded_at DESC, id DESC
        LIMIT 1
        '
    );

    $position = $positionStatement->fetch();

    /*
     * Use floor 1 only as an initial fallback when the
     * elevator_position table has not been initialized.
     */
    $currentFloor = $position
        ? (int) $position['current_floor']
        : 1;

    $lastFloor = $position
        ? (int) $position['last_floor']
        : 1;

    $controllerLabel =
        $controllerLabels[$controllerType];

    $message = sprintf(
        '%s requested floor %d',
        $controllerLabel,
        $requestedFloor
    );

    /*
     * Store the request as a CAN transaction.
     *
     * data stores the requested floor number.
     * sent = 1 means the request was sent.
     */
$insertStatement = $db->prepare(
    '
    INSERT INTO can_transaction (
        sent_by,
        transceived_at,
        data,
        message,
        current_floor,
        last_floor
    )
    VALUES (
        :sent_by,
        NOW(),
        :data,
        :message,
        :current_floor,
        :last_floor
    )
    '
);

$insertStatement->execute([
    'sent_by' => $sentByCanId,
    'data' => $requestedFloor,
    'message' => $message,
    'current_floor' => $currentFloor,
    'last_floor' => $lastFloor
]);

    sendJsonResponse(201, [
        'success' => true,
        'message' => 'The floor request was saved.',
        'transaction_id' => (int) $db->lastInsertId(),
        'requested_floor' => $requestedFloor,
        'controller_type' => $controllerType,
        'controller_label' => $controllerLabel,
        'current_floor' => $currentFloor,
        'last_floor' => $lastFloor
    ]);
} catch (JsonException) {
    sendJsonResponse(400, [
        'success' => false,
        'message' => 'Invalid JSON was received.'
    ]);
} catch (InvalidArgumentException $exception) {
    sendJsonResponse(422, [
        'success' => false,
        'message' => $exception->getMessage()
    ]);
} catch (PDOException $exception) {
    sendJsonResponse(500, [
        'success' => false,
        'message' => 'The database request failed.',
        'debug' => $exception->getMessage()
    ]);
} catch (Throwable $exception) {
    sendJsonResponse(500, [
        'success' => false,
        'message' => 'An unexpected server error occurred.',
        'debug' => $exception->getMessage()
    ]);
}
