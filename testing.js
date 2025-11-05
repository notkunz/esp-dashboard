// Import Firebase modules
import { initializeApp } from "https://www.gstatic.com/firebasejs/9.6.10/firebase-app.js";
import { getDatabase, ref, set, onValue, get, child } from "https://www.gstatic.com/firebasejs/9.6.10/firebase-database.js";

// ====== FIREBASE CONFIGURATION ======
// ⚠️ Replace all values below with your Firebase project's actual credentials
const firebaseConfig = {
  apiKey: "AIzaSyBtVI6cUmGCkN4fejlr4gxnsZdnswGSYR4",
  authDomain: "esp-dashboard-50f8b.firebaseapp.com",
  databaseURL: "https://esp-dashboard-50f8b-default-rtdb.firebaseio.com",
  projectId: "esp-dashboard-50f8b",
  storageBucket: "esp-dashboard-50f8b.firebasestorage.app",
  messagingSenderId: "849745049499",
  appId: "1:849745049499:web:c392ba3766f62b812a133e"
};

// Initialize Firebase app and database
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

// ====== FETCH & DISPLAY SLAVE DATA ======
function fetchSlavesFromFirebase() {
  const slavesRef = ref(db, "slaves");
  onValue(slavesRef, (snapshot) => {
    if (snapshot.exists()) {
      const slaves = snapshot.val();
      displaySlaves(Object.values(slaves));
    } else {
      displaySlaves([]);
    }
  });
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

// ====== TOGGLE SLAVE PIN ======
async function toggleSlave(id, pin) {
  const commandRef = ref(db, `commands/slaves/${id}/gpio${pin}`);
  const currentSnapshot = await get(commandRef);
  const currentVal = currentSnapshot.exists() ? currentSnapshot.val() : 0;
  const newValue = currentVal === 1 ? 0 : 1;
  await set(commandRef, newValue);
  console.log(`Toggled Slave ${id} GPIO${pin} → ${newValue}`);
}

// ====== TOGGLE MASTER PIN ======
async function toggleMaster(pin) {
  const btn = document.getElementById(`gpio${pin}`);
  btn.classList.toggle("on");
  btn.classList.toggle("off");

  const commandRef = ref(db, `commands/master/gpio${pin}`);
  const currentSnapshot = await get(commandRef);
  const currentVal = currentSnapshot.exists() ? currentSnapshot.val() : 0;
  const newValue = currentVal === 1 ? 0 : 1;
  await set(commandRef, newValue);
  console.log(`Toggled Master GPIO${pin} → ${newValue}`);
}

// ====== LISTEN FOR STATUS UPDATES FROM MASTER ======
const masterStatusRef = ref(db, "status/master");
onValue(masterStatusRef, (snapshot) => {
  if (snapshot.exists()) {
    const data = snapshot.val();
    Object.keys(data).forEach(key => {
      const pin = key.replace("gpio", "");
      const btn = document.getElementById(`gpio${pin}`);
      if (btn) {
        btn.classList.toggle("on", data[key] === 1);
        btn.classList.toggle("off", data[key] === 0);
      }
    });
  }
});

// ====== INITIAL LOAD ======
window.onload = fetchSlavesFromFirebase;
