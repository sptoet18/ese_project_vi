
// Elevator

document.getElementById('elevatorButton').addEventListener('click', function() {
    fetch('/php/modes/elevator.php')
        .then(response => response.text())
        .then(data => {
            //alert("response: " + data);
        })
        .catch(error => console.error('Error:', error));
});

// Sabbath

document.getElementById('sabbathButton').addEventListener('click', function() {
    fetch('/php/modes/sabbath.php')
        .then(response => response.text())
        .then(data => {
            //alert("response: " + data);
        })
        .catch(error => console.error('Error:', error));
});

// Maintenance

document.getElementById('maintenanceButton').addEventListener('click', function() {
    fetch('/php/modes/maintenance.php')
        .then(response => response.text())
        .then(data => {
            //alert("response: " + data);
        })
        .catch(error => console.error('Error:', error));
});