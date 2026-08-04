<?php

namespace objects;

require_once '../util.php';

class CanTransaction
{
    private int $id;
    private CanNode $sentBy;
    private \DateTimeImmutable $transceivedAt;
    private int $data;
    private string $message;
    private int $currentFloor;
    private int $lastFloor;

    private function __construct() {}

    public static function create(
        int $id,
        CanNode $sentBy,
        int $data,
        string $message,
        int $currentFloor,
        int $lastFloor
    ) {
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
                'sent_by' => $sentBy,
                'data' => $data,
                'message' => $message,
                'current_floor' => $currentFloor,
                'last_floor' => $lastFloor
            ]);

            $id = $db->lastInsertId();
        } catch (\PDOException $e) {
            die("Creation failed: " . $e->getMessage());
        }

        $canTransaction = self::get($id);

        if ($canTransaction === null) {
            throw new \RuntimeException("CAN Transaction {$id} was created but could not be found.");
        }

        return $canTransaction;
    }

    public static function get(int $id) {
        $db = connect();

        try {
            $query = '
                SELECT *
                FROM can_transaction
                WHERE id = :id
                LIMIT 1;
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'id' => $id
            ]);
            $row = $statement->fetch();
        } catch (\PDOException $e) {
            die("Get failed: " . $e->getMessage());
        }

        return $row ? self::fromRow($row) : null;
    }

    public function getId() : int {
        return $this->id;
    }

    public function getSentBy() : CanNode {
        return $this->sentBy;
    }

    public function getTransceivedAt() : \DateTimeImmutable {
        return $this->transceivedAt;
    }

    public function getData() : int {
        return $this->data;
    }

    public function getMessage() : string {
        return $this->message;
    }

    public function getCurrentFloor() : int {
        return $this->currentFloor;
    }

    public function getLastFloor() : int {
        return $this->lastFloor;
    }
    private static function fromRow(array $row) : CanTransaction {
        $canTransaction = new CanTransaction();
        $canTransaction->id = (int) $row['id'];
        $canTransaction->sentBy = CanNode::get($row['sent_by']->id);
        $canTransaction->data = (int) $row['data'];
        $canTransaction->message = (string) $row['message'];
        $canTransaction->currentFloor = (int) $row['current_floor'];
        $canTransaction->lastFloor = (int) $row['last_floor'];

        return $canTransaction;
    }
}