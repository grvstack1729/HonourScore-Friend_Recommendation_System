/*
 * ------------------------------------------------------------------------------------
 *  storage.cpp  —  CSV persistence layer
 *
 *  users.csv columns:
 *    userId,name,age,school,hobby,profession,
 *    interest,honorScore,isAdmin,password
 *
 *  friendships.csv columns:
 *    user1Id,user2Id,weight
 *
 *  FriendInteractions2.csv columns (original):
 *    user1,user2,num_interactions,
 *    last_interaction_days_ago,duration_minutes,
 *    interaction_score_mean
 * ------------------------------------------------------------------------------------
 */

#include "storage.h"
#include "types.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>

using namespace std;

// Global definitions ─────
unordered_map<string, User>                         users;
unordered_map<string, string>                       nameToId;
unordered_map<string, vector<pair<string,double>>>  graph;
unordered_map<string, set<string>>                  pendingReqs;

const string USERS_FILE       = "users.csv";
const string FRIENDSHIPS_FILE = "friendships.csv";
const string INTERACTIONS_FILE= "FriendInteractions2.csv";

// Users ──

void loadUsers() {
    ifstream f(USERS_FILE);
    if (!f.is_open()) return;
    string line;
    getline(f, line); // skip header
    while (getline(f, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line);
        User u;
        string tok;
        getline(ss, u.userId,     ',');
        getline(ss, u.name,       ',');
        getline(ss, tok,          ','); try { u.age = stoi(tok); } catch(...){ u.age=0; }
        getline(ss, u.school,     ',');
        getline(ss, u.hobby,      ',');
        getline(ss, u.profession, ',');
        getline(ss, u.interest,   ',');
        getline(ss, tok,          ','); try { u.honorScore = stoi(tok); } catch(...){ u.honorScore=60; }
        getline(ss, tok,          ','); u.isAdmin = (tok == "1");
        getline(ss, u.password);

        u.userId   = trim(u.userId);
        u.name     = trim(u.name);
        u.password = trim(u.password);
        if (!u.userId.empty()) {
            users[u.userId]   = u;
            nameToId[u.name]  = u.userId;
        }
    }
}

void saveUsers() {
    ofstream f(USERS_FILE);
    f << "userId,name,age,school,hobby,profession,interest,honorScore,isAdmin,password\n";
    for (auto& kv : users) {
        const User& u = kv.second;
        f << u.userId     << ","
          << u.name       << ","
          << u.age        << ","
          << u.school     << ","
          << u.hobby      << ","
          << u.profession << ","
          << u.interest   << ","
          << u.honorScore << ","
          << (u.isAdmin ? "1" : "0") << ","
          << u.password   << "\n";
    }
}

// Friendships ────

void loadFriendships() {
    graph.clear();
    ifstream f(FRIENDSHIPS_FILE);
    if (!f.is_open()) return;
    string line;
    getline(f, line); // header
    while (getline(f, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line);
        string u1, u2, wt;
        getline(ss, u1, ',');
        getline(ss, u2, ',');
        getline(ss, wt);
        double w = 1.0;
        try { w = stod(wt); } catch(...) {}
        u1 = trim(u1); u2 = trim(u2);
        if (u1.empty() || u2.empty()) continue;
        graph[u1].push_back({u2, w});
        graph[u2].push_back({u1, w});
    }
}

void saveFriendships() {
    ofstream f(FRIENDSHIPS_FILE);
    f << "user1Id,user2Id,weight\n";
    set<pair<string,string>> written;
    for (auto& kv : graph) {
        for (auto& edge : kv.second) {
            string a = kv.first, b = edge.first;
            if (a > b) swap(a, b);
            if (written.count({a, b})) continue;
            written.insert({a, b});
            f << a << "," << b << "," << edge.second << "\n";
        }
    }
}

// Seed from original interactions CSV ──────
/*
 * This only adds graph edges; it does NOT create user accounts.
 * Edge weights come from `interaction_score_mean` column.
 * If we already have the users in our system by name, we map
 * their names to userIds and add the edge.
 */
void loadInteractionsCSV(const string& filename) {
    ifstream f(filename);
    if (!f.is_open()) {
        cout << "  [!] Could not open " << filename << "\n";
        return;
    }
    string line;
    getline(f, line); // header

    while (getline(f, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line);
        string n1, n2, tok;
        double score = 1.0;

        getline(ss, n1, ',');
        getline(ss, n2, ',');
        for (int i = 0; i < 3; ++i) getline(ss, tok, ',');
        getline(ss, tok, ',');
        try { score = stod(tok); } catch (...) {}

        n1 = trim(n1); n2 = trim(n2);

        // Map to userIds if both users exist in our system
        if (nameToId.count(n1) && nameToId.count(n2)) {
            string id1 = nameToId[n1], id2 = nameToId[n2];
            graph[id1].push_back({id2, score});
            graph[id2].push_back({id1, score});
        }
    }
}