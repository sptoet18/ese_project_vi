<?php

namespace objects;

require_once '../util.php';

class canTransaction
{
    private int $id;
    private CanNode $sentBy;
    private \DateTimeImmutable $transceivedAt;
    private int $data;
    private string $message;
    private int $currentFloor;
    private int $lastFloor;

    public function __construct(
        CanNode $sentBy,
        int $data,
        string $message,
        int $currentFloor,
        int $lastFloor
    ) {
        $this->sentBy = $sentBy;
        $this->data = $data;
        $this->message = $message;
        $this->currentFloor = $currentFloor;
        $this->lastFloor = $lastFloor;
    }

    public function create() {
        $db = connect();

        try {
            $query = '
                INSERT INTO can_transaction(
                                            sent_by, 
                                            data, 
                                            message, 
                                            current_floor, 
                                            last_floor
                ) VALUES (
                          :sent_by,
                          :data,
                          :message,
                          :current_floor,
                          :last_floor
                );
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'sent_by' => $this->sentBy,
                'data' => $this->data,
                'message' => $this->message,
                'current_floor' => $this->currentFloor,
                'last_floor' => $this->lastFloor
            ]);

            $id = $db->lastInsertId();
        } catch (\PDOException $e) {
            die("Creation failed: " . $e->getMessage());
        }

        return $id;
    }
}