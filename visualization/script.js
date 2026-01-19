const nodes = ["client", "relay1", "relay2", "relay3", "server"];
let lastEventCount = 0;

function setNodeState(node, state) {
    const el = document.getElementById(node);
    el.className = "node " + state;
}

function addLog(text) {
    const log = document.getElementById("log");
    const div = document.createElement("div");
    div.className = "log-entry";
    div.textContent = text;
    log.appendChild(div);
    log.scrollTop = log.scrollHeight;
}

async function fetchEvents() {
    try {
        const res = await fetch("events.json?cache=" + Date.now());
        const events = await res.json();

        if (events.length === lastEventCount) return;

        for (let i = lastEventCount; i < events.length; i++) {
            const e = events[i];
            addLog(`[${e.timestamp}] ${e.node.toUpperCase()}: ${e.event}`);

            if (e.event.includes("start")) {
                setNodeState(e.node, "processing");
            }
            if (e.event.includes("success")) {
                setNodeState(e.node, "success");
            }
            if (e.event.includes("fail")) {
                setNodeState(e.node, "failed");
                document.getElementById("status").textContent = "FAILED";
            }
            if (e.event === "file_received") {
                document.getElementById("status").textContent = "COMPLETED";
                document.getElementById("filesize").textContent = e.details;
                setNodeState("server", "success");
            }
        }

        lastEventCount = events.length;
    } catch (err) {
        console.error("Error reading events.json");
    }
}

setInterval(fetchEvents, 1000);
