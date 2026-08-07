"use strict";

const eventSource = new EventSource("/php/sse.php");

const floorImage = document.getElementById("floor-indicator");
const modeChip = document.getElementById("mode-chip");
const stateChip = document.getElementById("state-chip");
const floorButtons = document.querySelectorAll(".floor-request");

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
     * Sabbath mode honours no requests at all, so lock the buttons out from
     * LIVE state rather than from whatever the mode was at page load.
     */
    floorButtons.forEach((button) => {
        button.disabled = mode === "sabbath";
    });
};

eventSource.onerror = (err) => {
    console.error("EventSource failed:", err);
};
