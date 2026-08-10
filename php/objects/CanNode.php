<?php

namespace objects;

require_once '../util.php';

class CanNode
{
    private int $id;
    private int $canId;
    private string $name;
    private \DateTimeImmutable $createdAt;
    private bool $archived;

    public function __construct() {}

    public static function create(int $canId, string $name) {
        $db = connect();

        try {
            $query = '
                INSERT INTO can_node(
                                     can_id, 
                                     name
                ) VALUES (
                          :can_id,
                          :name
                );
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'can_id' => $canId,
                'name' => $name,
            ]);

            $id = $db->lastInsertId();
        } catch (\PDOException $e) {
            die("Creation failed: " . $e->getMessage());
        }

        $canNode = self::get($id);

        if ($canNode === null) {
            throw new \RuntimeException("CAN Node {$id} was created but could not be found.");
        }

        return $canNode;
    }

    public static function get(int $id) : ?self {
        $db = connect();

        try {
            $query = '
                SELECT *
                FROM can_node
                WHERE id = :id
                    AND archived = false
                LIMIT 1
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'id' => $id
            ]);
            $row = $statement->fetch(\PDO::FETCH_ASSOC);
        } catch (\PDOException $e) {
            die ("Get failed: " . $e->getMessage());
        }

        return $row ? self::fromRow($row) : null;
    }

    public static function getByCanId(int $canId) : ?self {
        $db = connect();

        try {
            $query = '
                SELECT *
                FROM can_node
                WHERE can_id = :can_id
                    AND archived = false
                LIMIT 1
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'can_id' => $canId
            ]);
            $row = $statement->fetch();
        } catch (\PDOException $e) {
            die ("Get failed: " . $e->getMessage());
        }

        return $row ? self::fromRow($row) : null;
    }

    public function getId() : int {
        return $this->id;
    }

    public function getCanId() : int {
        return $this->canId;
    }

    public function getName() : string {
        return $this->name;
    }

    public function getCreatedAt() : \DateTimeImmutable {
        return $this->createdAt;
    }

    public function getArchived() : bool {
        return $this->archived;
    }

    private static function fromRow(array $row) : self {
        $canNode = new self();
        $canNode->id = (int) $row['id'];
        $canNode->canId = (int) $row['can_id'];
        $canNode->name = (string) $row['name'];
        $canNode->createdAt = new \DateTimeImmutable($row['created_at']);
        $canNode->archived = (bool) $row['archived'];

        return $canNode;
    }
}