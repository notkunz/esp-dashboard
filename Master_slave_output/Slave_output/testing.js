const MASTER_IP = "192.168.4.1";

async function fetchSlaves() {
  try {
    const res = await fetch(`http://${MASTER_IP}/json`);
    const data = await res.json();
    displaySlaves(data);
  } catch (err) {
    console.error("Error fetching slave data:", err);
  }
}

function displaySlaves(slaves) {
  const container = document.getElementById("slaves");
  container.innerHTML = "";

  if (slaves.length === 0) {
    container.innerHTML = "<p>No slaves connected</p>";
    return;
  }

  slaves.forEach(slave => {
    const card = document.createElement("div");
    card.className = "slave-card";

    const header = document.createElement("div");
    header.className = "slave-header";
    header.textContent = `Slave #${slave.id}`;

    const status = document.createElement("div");
    status.className = "slave-status";
    status.textContent = `Last seen: ${(slave.seen / 1000).toFixed(1)}s ago`;

    const gpioContainer = document.createElement("div");
    gpioContainer.className = "slave-gpios";

    slave.s.forEach((val, i) => {
      const btn = document.createElement("button");
      btn.className = "slave-btn " + (val === 1 ? "on" : "off");
      btn.textContent = `GPIO${i}`;
      btn.onclick = () => toggleSlave(slave.id, i);
      gpioContainer.appendChild(btn);
    });

    card.appendChild(header);
    card.appendChild(status);
    card.appendChild(gpioContainer);
    container.appendChild(card);
  });
}

async function toggleSlave(id, pin) {
  try {
    await fetch(`http://${MASTER_IP}/toggle_slave?id=${id}&pin=${pin}`);
    setTimeout(fetchSlaves, 300);
  } catch (err) {
    console.error("Error toggling slave:", err);
  }
}

async function toggleMaster(pin) {
  const btn = document.getElementById(`gpio${pin}`);
  btn.classList.toggle("on");
  btn.classList.toggle("off");

  try {
    await fetch(`http://${MASTER_IP}/toggle?pin=${pin}`);
  } catch (err) {
    console.error("Error toggling master pin:", err);
  }
}

setInterval(fetchSlaves, 1000);
window.onload = fetchSlaves;

