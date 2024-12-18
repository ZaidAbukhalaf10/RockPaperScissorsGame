const ws = new WebSocket("ws://localhost:8080", "game-protocol");

ws.onopen = () => {
    console.log("WebSocket connection established");
};

ws.onmessage = (event) => {
    const message = event.data;

    if (message.startsWith("Connected clients:")) {
        document.getElementById("client-count").innerText = message;
    } else {
        document.getElementById("result").innerText = message;
    }
};

function play(choice) {
    ws.send(choice);
}
ws.onclose = () => {
    console.log("WebSocket connection closed. Reconnecting...");
    // Optionally, you can add automatic reconnection logic here if needed
};
