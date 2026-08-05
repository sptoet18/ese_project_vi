
// Elevator
document.getElementById('elevatorButton').addEventListener('click', function() {
    fetch('/php/modes/elevator.php')
        .catch(error => console.error('Error:', error));

    window.location.reload();
});

// Sabbath
document.getElementById('sabbathButton').addEventListener('click', function() {
    fetch('/php/modes/sabbath.php')
        .catch(error => console.error('Error:', error));
    
    window.location.reload();
});

// Maintenance
document.getElementById('maintenanceButton').addEventListener('click', function() {
    fetch('/php/modes/maintenance.php')
        .catch(error => console.error('Error:', error));
    
    window.location.reload();
});
