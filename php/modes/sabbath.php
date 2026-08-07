<?php
    require_once __DIR__ . '/../util.php';

    session_start();

    if (!isset($_SESSION['username'])) {
        return;
    }

    $db = dbConnect('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');

    $previousQuery = $db->prepare('
        select *
        from elevator_position
        order by id desc
        limit 1
    ');
    $previousQuery->execute([]);
    $previousInfo = $previousQuery->fetch();
    $query = $db->prepare('
        insert into elevator_position (
            current_floor,
            last_floor,
            is_moving,
            is_closed,
            mode
        ) values (
            :current_floor,
            :last_floor,
            :is_moving,
            :is_closed,
            :mode
        )
    ');
    $query->execute([
        'current_floor' => $previousInfo['current_floor'],
        'last_floor' => $previousInfo['last_floor'],
        'is_moving' => $previousInfo['is_moving'],
        'is_closed' => $previousInfo['is_closed'],
        'mode' => 'sabbath'
    ]);
?>