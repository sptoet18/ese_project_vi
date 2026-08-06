"use strict";

document.addEventListener("DOMContentLoaded", () => {
    const floorSlider =
        document.getElementById("floor-slider");

    const selectedFloorOutput =
        document.getElementById("selected-floor");

    const sendFloorButton =
        document.getElementById("send-floor-request");

    const statusOutput =
        document.getElementById("slider-status");

    if (
        !floorSlider ||
        !selectedFloorOutput ||
        !sendFloorButton ||
        !statusOutput
    ) {
        console.error(
            "One or more elevator slider elements could not be found."
        );

        return;
    }

    function updateSelectedFloor()
    {
        selectedFloorOutput.textContent =
            floorSlider.value;
    }
    function wheelSlide(event)
    {
        //stop the page from scrolling
        event.preventDefault();
        if (floorSlider.disabled) {
            return;
        }
        let selectedFloor = Number.parseInt(floorSlider.value, 10);

        if (event.deltaY < 0) {
            selectedFloor+=1;
        } else if (event.deltaY > 0) {
            selectedFloor-=1;
        }

        selectedFloor = Math.min(Number(floorSlider.max), Math.max(Number(floorSlider.min), selectedFloor));
        floorSlider.value = selectedFloor.toString();
        updateSelectedFloor();
    }
    


    async function sendFloorRequest()
    {
        const requestedFloor =
            Number.parseInt(floorSlider.value, 10);

        if (
            !Number.isInteger(requestedFloor) ||
            requestedFloor < 1 ||
            requestedFloor > 3
        ) {
            statusOutput.textContent =
                "Please select a valid floor.";

            return;
        }

        floorSlider.disabled = true;
        sendFloorButton.disabled = true;

        statusOutput.textContent =
            `Sending floor ${requestedFloor} request...`;

        try {
            const response = await fetch(
                "request-floor.php",
                {
                    method: "POST",

                    headers: {
                        "Content-Type": "application/json",
                        "Accept": "application/json"
                    },

                    body: JSON.stringify({
                        requested_floor: requestedFloor,
                        controller_type: "car_controller"
                    })
                }
            );

            const responseText =
                await response.text();

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
                    "The floor request could not be saved."
                );
            }

            statusOutput.textContent =
                `Floor ${result.requested_floor} request was saved.`;

            console.log(
                "Slider transaction saved:",
                result
            );
        } catch (error) {
            console.error(error);

            statusOutput.textContent =
                `Request failed: ${error.message}`;
        } finally {
            floorSlider.disabled = false;
            sendFloorButton.disabled = false;
        }
    }

    floorSlider.addEventListener(
        "input",
        updateSelectedFloor
    );

    sendFloorButton.addEventListener(
        "click",
        sendFloorRequest
    );

    updateSelectedFloor();
});