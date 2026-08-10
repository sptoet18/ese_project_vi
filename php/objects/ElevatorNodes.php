<?php

namespace objects;

class FloorControllerNode extends CanNode
{
    private int $floor;

    public function __construct(int $canId, int $name, int $floor) {
        $this->floor = $floor;

        // Get or create the parent
        $canNode = parent::getByCanId($canId);
        if (!$canNode) {
            $canNode = parent::create($canId, $name);
        }

        return $canNode;
    }

    public function getCanId(): int {
        return $this->floor;
    }
}

class CarControllerNode extends CanNode
{
    private int $currentFloor;
    private int $isClosed;
    private int $isMoving;

    public function __construct(int $canId, int $name, int $currentFloor, bool $isClosed, bool $isMoving) {
        $this->currentFloor = $currentFloor;
        $this->isClosed = $isClosed;
        $this->isMoving = $isMoving;

        // Get or create the parent
        $canNode = parent::getByCanId($canId);
        if (!$canNode) {
            $canNode = parent::create($canId, $name);
        }

        return $canNode;
    }

    public function getCurrentFloor(): int {
        return $this->currentFloor;
    }

    public function isClosed(): bool {
        return $this->isClosed;
    }

    public function isMoving(): bool {
        return $this->isMoving;
    }
}