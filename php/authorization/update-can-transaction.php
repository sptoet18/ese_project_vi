<?php

header('Content-Type: application/json');

error_reporting(E_ALL);
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);

require_once "../objects/CanTransaction.php";
use objects\CanTransaction;

session_start();

// Check for user
if (!isset($_SESSION['username'])) {
    echo json_encode(['success' => false, 'error' => 'You are not authorized to access this page.']);
    exit;
}

// Get the data
$input = json_decode(file_get_contents('php://input'), true);

$id = $input["id"];

// ID is required to get the transaction
if (!$id) {
    echo json_encode(['success' => false, 'error' => 'Must include ID']);
    exit;
}

$sentBy = $input['sentBy'] ?? null;
$data = $input['data'] ?? null;
$message = $input['message'] ?? null;
$currentFloor = $input['current_floor'] ?? null;
$lastFloor = $input['last_floor'] ?? null;

// Update the row
CanTransaction::updateById($id, $sentBy, $data, $message, $currentFloor, $lastFloor);

echo json_encode(['success' => true]);
