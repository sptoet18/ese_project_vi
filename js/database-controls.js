
// Get all the checkboxes
document.querySelectorAll('.editing-toggle').forEach(element => {
    // Check for a click
    element.addEventListener('change', function(event) {
        if (event.target.checked) {
            // checked

            const row = this.closest('tr');
            // Get all the editable fields in the row
            const columns = row.querySelectorAll('.editable');
            const saveButton = row.querySelector('.save-button');

            columns.forEach(column => {
                // Save the original data
                column.dataset.original = column.textContent.trim();
                field = column.dataset.field;

                // Make the column editable
                column.innerHTML = `<input type="text" class="editable" data-field="${field}" value="${column.dataset.original}" style="width: 100%;">`;
            })

            // Show the save button
            saveButton.style.display = "inline";
        } else if (!event.target.checked) {
            // unchecked

            const row = this.closest('tr');
            // Get all the editable fields in the row
            const columns = row.querySelectorAll('.editable');
            const saveButton = row.querySelector('.save-button');

            columns.forEach(column => {
                // Put the original database
                column.innerHTML = `<p class="editable">${column.dataset.original}</p>`
            })

            // Show the save button
            saveButton.style.display = "none";
        }

    })
})

document.querySelectorAll('.save-button').forEach(button => {
    button.addEventListener('click', function () {
        const row = this.closest('tr');
        // Get all the editable fields in the row
        const columns = row.querySelectorAll('.editable');
        const idColumn = row.querySelector('.id');
        const id = idColumn.textContent.trim();

        const data = { id: id };

        columns.forEach(column => {
            const dataset = column.dataset;
            if (!!column.value) {
                // console.log("value:", column.value, "field:", dataset.field);
                data[dataset.field] = column.value;
            }
        })

        fetch('/php/authorization/update-can-transaction.php', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        })
            .then(res => res.json())
            .then(response => {
                if (response.success) {
                    columns.forEach(column => {
                        // Make text uneditable
                        column.innerHTML = `<p class="editable">${column.dataset.original}</p>`;
                        const dataset = column.dataset;
                        column.textContent = data[dataset.field];
                    })

                    // Uncheck the box
                    const checkbox = row.querySelector('.editing-toggle');
                    checkbox.checked = false;
                }
            })
        .catch(err => alert('Error: ' + err));
    })
})
