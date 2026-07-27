"use strict";

        document.addEventListener("DOMContentLoaded", () => {
            const requestButtons =
                document.querySelectorAll(".floor-request");

            const statusOutput =
                document.getElementById("request-status");

            requestButtons.forEach((button) => {
                button.addEventListener("click", async () => {
                    const requestedFloor =
                        Number(button.dataset.floor);

                    const controllerType =
                        button.dataset.controller;

                    // Prevent repeated clicks while request is running.
                    requestButtons.forEach((currentButton) => {
                        currentButton.disabled = true;
                    });

                    statusOutput.textContent =
                        `Sending floor ${requestedFloor} request...`;

                    try {
                        
                        const response = await fetch(
                            "../../php/authorization/request-floor.php",
                            {
                                method: "POST",
                                headers: {
                                    "Content-Type": "application/json",
                                    "Accept": "application/json"
                                },
                                body: JSON.stringify({
                                    requested_floor: requestedFloor,
                                    controller_type: controllerType
                                })
                            }
                        );

                        /*
                         * Read as text first. This gives a more useful
                         * error when PHP outputs HTML or a warning.
                         */
                        const responseText = await response.text();

                        let result;

                        try {
                            result = JSON.parse(responseText);
                        } catch {
                            throw new Error(
                                "The server did not return valid JSON: " +
                                responseText
                            );
                        }

                        if (!response.ok || !result.success) {
                            throw new Error(
                                result.debug ??
                                result.message ??
                                "The request could not be saved."
                            );
                        }

                        statusOutput.textContent =
                            `${result.controller_label} request for ` +
                            `floor ${result.requested_floor} was saved.`;

                        console.log("Saved transaction:", result);
                    } catch (error) {
                        console.error(error);

                        statusOutput.textContent =
                            `Request failed: ${error.message}`;
                    } finally {
                        requestButtons.forEach((currentButton) => {
                            currentButton.disabled = false;
                        });
                    }
                });
            });
        });