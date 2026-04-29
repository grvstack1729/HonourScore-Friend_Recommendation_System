# Ignore compiled executables
*.exe

# Ignore VS Code settings 
.vscode/


# 🤝 FriendGraph: DAA Friend Recommendation System

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Vanilla JS](https://img.shields.io/badge/Frontend-Vanilla_JS-yellow.svg)

FriendGraph is a high-performance, graph-based social network and recommendation engine built entirely in C++ from scratch. It uses core Design and Analysis of Algorithms (DAA) concepts to simulate a social network, calculate dynamic honor scores, and provide real-time friend recommendations via a local REST API and Web GUI.

## 🚀 Core DAA Algorithms
* **Graph Traversal (Breadth-First Search):** Explores the social network up to Depth 2 (friends of friends) in `O(V + E)` time to find recommendation candidates without searching the entire database.
* **Sorting (Max-Heap / Priority Queue):** Efficiently sorts and extracts the Top 5 best matches based on a composite compatibility score in `O(K log N)` time.
* **Storage (Hash Maps):** Utilizes `std::unordered_map` for `O(1)` average time complexity for user authentication and data retrieval.

## 🛠️ Architecture & Tech Stack
* **Backend:** C++17 
* **API/Networking:** `cpp-httplib` (Local HTTP Web Server on Port 8080)
* **Frontend:** HTML5, CSS3, Vanilla JavaScript (Fetch API)
* **Database:** Custom File I/O (CSV based persistence layer)

## 👥 Team Members & Roles
* **Garv Kumar Singh** - GUI Development, Systems Integration, & Deployment (Team Lead)
* **Abhishek Sharma** - Core DAA Algorithms (BFS & Max-Heaps)
* **Krish Jaisingh** - Honor Score Mathematical Modeling & State Management
* **Sakshi Kalkar** - System Validation, UI Enhancements & Sample Data Generation

## ⚙️ How to Build and Run (Windows)

**1. Clone the repository:**
\`\`\`bash
git clone https://github.com/grvstack1729/HonourScore-Friend_Recommendation_System.git
cd daa-friend-recommendation
\`\`\`

**2. Compile the C++ Backend:**
Make sure you have GCC/MinGW installed with POSIX thread support.
\`\`\`powershell
g++ -std=c++17 -Wall -O2 server.cpp auth.cpp graph_ops.cpp honor.cpp recommender.cpp storage.cpp utils.cpp Reports.cpp -o server.exe -pthread
\`\`\`

**3. Start the Server:**
\`\`\`powershell
.\server.exe
\`\`\`

**4. Launch the GUI:**
Double-click `index.html` in your file explorer to open the dashboard in your web browser. 

**Demo Credentials as Administrator:**
* User ID: `ADMIN`
* Password: `admin123`

**Demo Creditional as User:**
* User Id: `U1001`
* Password: `arjun123`

<img width="2559" height="1398" alt="Screenshot 2026-04-29 214759" src="https://github.com/user-attachments/assets/cbddb467-b0cf-4237-b27d-5fed8d3f0644" />
<img width="2559" height="1402" alt="Screenshot 2026-04-29 214744" src="https://github.com/user-attachments/assets/9e91a42c-b575-40fc-ba28-cc747ad6003f" />
