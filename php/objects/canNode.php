<?php

namespace objects;

class canNode
{
    private int $id;
    private int $canId;
    private string $name;
    private \DateTimeImmutable $createdAt;
    private bool $archived;

    public function __construct(int $canId, string $name) {
        $this->canId = $canId;
        $this->name = $name;
    }

    public function create() {
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
    
                SELECT LAST_INSERT_ID() AS id;
            ';
            $statement = $db->prepare($query);
            $statement->execute([
                'can_id' => $this->canId,
                'name' => $this->name,
            ]);

            $id = $db->lastInsertId();
        } catch (\PDOException $e) {
            die("Creation failed: " . $e->getMessage());
        }

        return $id;
    }
}