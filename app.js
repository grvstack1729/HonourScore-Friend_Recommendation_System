let DB = { users: {} };
let currentUser = null;

// Load from storage
function load() {
  const data = localStorage.getItem("users");
  if (data) DB.users = JSON.parse(data);
}

// Save to storage
function save() {
  localStorage.setItem("users", JSON.stringify(DB.users));
}

// Demo users
function seed() {
  DB.users = {
    "Wendy": { userId: "Wendy", password: "Wendy" },
    "Paul": { userId: "Paul", password: "Paul" },
    "ADMIN": { userId: "ADMIN", password: "admin123" }
  };

  save();
  alert("Demo users loaded!");
}

// Login
function login() {
  const uid = document.getElementById("uid").value.trim();
  const pwd = document.getElementById("pwd").value.trim();
  const err = document.getElementById("error");

  const user = DB.users[uid];

  if (!user || user.password !== pwd) {
    err.innerText = "Invalid user or password";
    return;
  }

  currentUser = uid;
  showDashboard();
}

// Show dashboard
function showDashboard() {
  document.getElementById("auth").style.display = "none";
  document.getElementById("dashboard").style.display = "block";
  document.getElementById("welcome").innerText = "Welcome " + currentUser;
}

// Logout
function logout() {
  currentUser = null;
  document.getElementById("auth").style.display = "block";
  document.getElementById("dashboard").style.display = "none";
}

// Init
load();