"use strict";

/*
 * Door open/close buttons.
 *
 * Kept apart from elevatorControl.js on purpose: that file binds ".floor-request"
 * and reads dataset.floor, so a door button carrying that class would post
 * requested_floor: NaN and get rejected by request-floor.php. The door buttons
 * carry ".door-request" and nothing else.
 */
document.addEventListener("DOMContentLoaded", () => {
    const doorButtons = document.querySelectorAll(".door-request");
    const statusOutput = document.getElementById("door-status");
    const modeChip = document.getElementById("mode-chip");
    const doorDiagram = document.getElementById("door-diagram");

    /*
     * The same two conditions door-control.php and the firmware refuse on.
     * Read from the DOM that event-source.js keeps current, so this always
     * agrees with what the elevator last reported.
     */
    const commandsLockedOut = () => {
        const inSabbath = modeChip && modeChip.dataset.mode === "sabbath";
        const inTransit = doorDiagram && doorDiagram.dataset.door === "moving";

        return Boolean(inSabbath || inTransit);
    };

    // Writes the message AND the state the CSS colours it by.
    const showStatus = (state, message) => {
        if (!statusOutput) {
            return;
        }

        statusOutput.textContent = message;
        statusOutput.dataset.state = state;
    };

    doorButtons.forEach((button) => {
        button.addEventListener("click", async () => {
            const door = button.dataset.door;

            // Prevent repeated clicks while the request is running.
            doorButtons.forEach((currentButton) => {
                currentButton.disabled = true;
            });

            showStatus("pending", `Sending ${door} door command...`);

            try {
                const response = await fetch(
                    "/php/authorization/door-control.php",
                    {
                        method: "POST",
                        headers: {
                            "Content-Type":
                                "application/x-www-form-urlencoded"
                        },
                        body: new URLSearchParams({ door: door })
                    }
                );

                /*
                 * door-control.php answers with plain pipe separated text:
                 *
                 *   ok|<message>   or   error|<message>
                 *
                 * Read as text either way - a PHP warning would land here too,
                 * and showing it beats a silent failure.
                 */
                const responseText = await response.text();

                const separator = responseText.indexOf("|");

                if (separator === -1) {
                    throw new Error(responseText);
                }

                const state = responseText.slice(0, separator);
                const message = responseText.slice(separator + 1);

                if (state !== "ok") {
                    throw new Error(message);
                }

                showStatus("ok", message);
            } catch (error) {
                console.error(error);

                showStatus("error", `Door command failed: ${error.message}`);
            } finally {
                /*
                 * Only hand the buttons back when the elevator still allows it.
                 * Re-enabling unconditionally would un-disable the Sabbath and
                 * mid-travel locks until the next SSE tick a second later.
                 */
                const lockedOut = commandsLockedOut();

                doorButtons.forEach((currentButton) => {
                    currentButton.disabled = lockedOut;
                });
            }
        });
    });
});
