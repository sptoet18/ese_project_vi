
const eventSource = new EventSource("/php/sse.php");

eventSource.onmessage = (event) => {
    const data = JSON.parse(event.data);
    const html = document.getElementById('sse');

    html.innerHTML += `<p>Floor: ${data.current_floor}</p>`;
};

eventSource.onerror = (err) => {
    console.error("EventSource failed:", err);
};

