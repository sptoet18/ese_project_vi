"use strict";

const eventSource = new EventSource("/php/sse.php");

const floorImage = document.getElementById("floor-indicator");
const modeChip = document.getElementById("mode-chip");
const stateChip = document.getElementById("state-chip");
const floorButtons = document.querySelectorAll(".floor-request");
const doorDiagram = document.getElementById("door-diagram");
const doorStateLabel = document.getElementById("door-state-label");
const doorButtons = document.querySelectorAll(".door-request");

eventSource.onmessage = (event) => {
    /*
     * sse.php sends one elevator_position row as five pipe separated
     * fields, in this order:
     *
     *   current_floor | last_floor | is_moving | is_closed | mode
     */
    const fields = event.data.split("|");

    const currentFloor = fields[0];
    const isMoving = Number(fields[2]);
    const isClosed = Number(fields[3]);
    const mode = fields[4];

    if (floorImage) {
        floorImage.src =
            `/assets/images/floor-indicators/floor${currentFloor}.png`;
    }

    if (modeChip) {
        modeChip.textContent = mode;
        modeChip.dataset.mode = mode;
    }

    if (stateChip) {
        stateChip.textContent =
            isMoving
                ? `Moving to floor ${currentFloor}`
                : `At floor ${currentFloor} - door ` +
                  (isClosed ? "closed" : "open");
    }

    /*
     * The door diagram is pure CSS - it animates off this one attribute, so all
     * that happens here is naming the state the firmware reports.
     *
     * "moving" is its own state rather than just "closed" because the car being
     * in transit is why the door buttons are locked out below.
     */
    const doorState = isMoving ? "moving" : (isClosed ? "closed" : "open");

    if (doorDiagram) {
        doorDiagram.dataset.door = doorState;
    }

    if (doorStateLabel) {
        doorStateLabel.textContent =
            isMoving
                ? "Car moving"
                : (isClosed ? "Door closed" : "Door open");

        doorStateLabel.dataset.door = doorState;
    }

    /*
     * Sabbath mode honours no requests at all, so lock the buttons out from
     * LIVE state rather than from whatever the mode was at page load.
     */
    floorButtons.forEach((button) => {
        button.disabled = mode === "sabbath";
    });

    /*
     * Door commands are refused in Sabbath and mid travel, by door-control.php
     * and by the firmware both. Grey the buttons out so the refusal is visible
     * before the click rather than after it.
     */
    doorButtons.forEach((button) => {
        button.disabled = mode === "sabbath" || isMoving === 1;
    });
};

eventSource.onerror = (err) => {
    console.error("EventSource failed:", err);
};
