
// Get all the checkboxes
document.querySelectorAll('.editing-toggle').forEach(i => {
    // Check for a click
    i.addEventListener('change', function(event) {
        if (event.target.checked) {
            console.log("Clicked");

            const row = this.closest('tr');
            // Get all the editable fields in the row
            const columns = row.querySelectorAll('.editable');
            const saveButton = row.querySelectorAll('.save-button');

            columns.forEach(column => {
                // Save the original data
                column.dataset.original = column.textContent;
                console.log("Original Data:", column.dataset.original);

                // Make the column editable
                column.innerHTML = `<input type="text" value="${column.dataset.original}">`;
            })

            // Show the save button
            saveButton.style.display = 'inline-block';
        } else if (!event.target.checked) {
            console.log("Unclicked");

            const row = this.closest('tr');
            // Get all the editable fields in the row
            const columns = row.querySelectorAll('.editable');
            const saveButton = row.querySelectorAll('.save-button');

            columns.forEach(column => {
                // Save the original data
                column.dataset.original = column.textContent;
                console.log("Original Data:", column.dataset.original);

                // Make the column editable
                column.innerHTML = `<input type="text" value="${column.dataset.original}">`;
            })

            // Show the save button
            saveButton.style.display = 'none';
        }

    })
})
