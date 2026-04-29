/*
 * ------------------------------------------------------------------------------------
 *  menus.cpp  —  Terminal UI menus
 * ------------------------------------------------------------------------------------
 */

#include "menus.h"
#include "types.h"
#include "utils.h"
#include "honor.h"
#include "graph_ops.h"
#include "recommender.h"
#include "storage.h"
#include <iostream>
#include <iomanip>
#include <queue>

using namespace std;

// Display helpers 

static void printUserCard(const User& u, bool showHonor = false) {
    cout << "  User ID    : " << u.userId     << "\n";
    cout << "  Name       : " << u.name       << "\n";
    cout << "  Age        : " << u.age        << "\n";
    cout << "  School     : " << u.school     << "\n";
    cout << "  Hobby      : " << u.hobby      << "\n";
    cout << "  Profession : " << u.profession << "\n";
    cout << "  Interest   : " << u.interest   << "\n";
    if (showHonor)
        cout << "  Honor Score: " << u.honorScore << "/100\n";
}

static void printFriendsList(const string& userId) {
    auto friends = getFriendsList(userId);
    if (friends.empty()) { cout << "  No friends yet.\n"; return; }
    int i = 1;
    for (auto& fid : friends) {
        if (!users.count(fid)) continue;
        cout << "  " << i++ << ". [" << fid << "] " << users[fid].name << "\n";
    }
}

// User Menu

void userMenu(const string& userId) {
    while (true) {
        User& me = users[userId];
        cout << "\n";
        divider('=');
        cout << "  Welcome, " << me.name << "  [" << userId << "]";
        if (shouldWarnUser(userId))
            cout << "  ⚠  Your account activity is low.";
        cout << "\n";
        divider();
        cout << "  1. View my profile\n";
        cout << "  2. View my friends\n";
        cout << "  3. Get friend recommendations\n";
        cout << "  4. Send a friend request\n";
        cout << "  5. View incoming requests\n";
        cout << "  6. Search a user\n";
        cout << "  7. Logout\n";
        divider();

        int ch = safeIntInput("  Choice: ", 1, 7);

        if (ch == 1) {
            divider();
            printUserCard(me);

        } else if (ch == 2) {
            divider();
            cout << "  Your friends:\n";
            printFriendsList(userId);

        } else if (ch == 3) {
            /*
             * DAA Core: BFS + Max-Heap
             * Scores candidates using composite weighted formula.
             */
            divider();
            cout << "  Calculating recommendations...\n\n";
            auto recs = recommendTopK(userId, 5);
            if (recs.empty()) {
                cout << "  No recommendations yet — add some friends first!\n";
            } else {
                cout << "  Top " << recs.size() << " recommendations for you:\n\n";
                for (int i = 0; i < (int)recs.size(); ++i) {
                    const User& c = users[recs[i].userId];
                    cout << "  " << (i+1) << ". " << c.name
                         << "  (" << c.age << " yrs, "
                         << c.hobby << ", " << c.profession << ")\n";
                    cout << "     ID: " << recs[i].userId
                         << "  Score: " << fixed << setprecision(3) << recs[i].score << "\n";
                }
            }
            adjustHonorScore(userId, +1, "used recommendations");
            saveUsers();

        } else if (ch == 4) {
            divider();
            cout << "  Enter user ID to send request: ";
            string target; getline(cin, target); target = trim(target);
            if (!users.count(target)) {
                cout << "  [!] User ID not found.\n";
            } else if (target == userId) {
                cout << "  [!] Cannot send a request to yourself.\n";
            } else if (areFriends(userId, target)) {
                cout << "  [!] You are already friends.\n";
            } else if (pendingReqs[userId].count(target)) {
                cout << "  [!] Request already sent.\n";
            } else {
                pendingReqs[userId].insert(target);
                cout << "  Request sent to " << users[target].name << ".\n";
                adjustHonorScore(userId, +1, "sent friend request");
                saveUsers();
            }

        } else if (ch == 5) {
            divider();
            cout << "  Incoming requests:\n";
            vector<string> senders;
            for (auto& kv : pendingReqs)
                if (kv.second.count(userId)) senders.push_back(kv.first);

            if (senders.empty()) { cout << "  None.\n"; }
            else {
                for (int i = 0; i < (int)senders.size(); ++i)
                    cout << "  " << (i+1) << ". "
                         << users[senders[i]].name << "  [" << senders[i] << "]\n";

                int pick = safeIntInput("\n  Accept which? (0 to skip): ", 0, (int)senders.size());
                if (pick > 0) {
                    string sender = senders[pick - 1];
                    addFriendship(userId, sender);
                    pendingReqs[sender].erase(userId);
                    cout << "  You are now friends with " << users[sender].name << "!\n";
                    adjustHonorScore(userId, +2, "accepted request");
                    saveUsers();
                }
            }

        } else if (ch == 6) {
            divider();
            cout << "  Enter name or user ID: ";
            string q; getline(cin, q); q = trim(q);
            string fid = "";
            if (users.count(q)) fid = q;
            else if (nameToId.count(q)) fid = nameToId[q];

            if (!fid.empty() && !users[fid].isAdmin) {
                cout << "\n  Profile:\n";
                printUserCard(users[fid]);
                if (areFriends(userId, fid))
                    cout << "  [You are friends]\n";
            } else {
                cout << "  User not found.\n";
            }

        } else {
            cout << "  Logged out.\n";
            return;
        }
    }
}

// Admin Menu

void adminMenu() {
    while (true) {
        cout << "\n";
        divider('=');
        cout << "  ADMIN PANEL\n";
        divider();
        cout << "  1. All users sorted by honor score\n";
        cout << "  2. View a user's profile & friends\n";
        cout << "  3. Adjust a user's honor score\n";
        cout << "  4. Delete a user\n";
        cout << "  5. Graph summary\n";
        cout << "  6. Logout\n";
        divider();

        int ch = safeIntInput("  Choice: ", 1, 6);

        if (ch == 1) {
            /*
             * DAA: Max-Heap sort
             * We push all (honorScore, userId) into a priority_queue
             * and pop in descending order — O(n log n).
             */
            divider();
            priority_queue<pair<int,string>> pq;
            for (auto& kv : users)
                if (!kv.second.isAdmin)
                    pq.push({kv.second.honorScore, kv.first});

            cout << "  " << left << setw(5) << "Rank"
                 << setw(10) << "UserID"
                 << setw(18) << "Name"
                 << setw(8)  << "Score"
                 << setw(5)  << "Age"
                 << "Hobby / Profession\n";
            divider('-', 65);
            int rank = 1;
            while (!pq.empty()) {
                pair<int, string> top = pq.top();
                pq.pop();
                int score = top.first;
                string uid = top.second;
                const User& u = users[uid];
                cout << "  " << setw(5) << rank++
                     << setw(10) << uid
                     << setw(18) << u.name
                     << setw(8)  << score
                     << setw(5)  << u.age
                     << u.hobby << " / " << u.profession << "\n";
            }

        } else if (ch == 2) {
            divider();
            cout << "  Enter user ID: ";
            string uid; getline(cin, uid); uid = trim(uid);
            if (!users.count(uid)) { cout << "  [!] Not found.\n"; }
            else {
                cout << "\n";
                printUserCard(users[uid], true);
                cout << "\n  Friends:\n";
                printFriendsList(uid);
            }

        } else if (ch == 3) {
            divider();
            cout << "  Enter user ID: ";
            string uid; getline(cin, uid); uid = trim(uid);
            if (!users.count(uid) || users[uid].isAdmin) { cout << "  [!] Not found.\n"; }
            else {
                cout << "  Current score: " << users[uid].honorScore << "\n";
                int delta = safeIntInput("  Change by (negative to penalise): ", -100, 100);
                adjustHonorScore(uid, delta, "admin adjustment");
                saveUsers();
                cout << "  New score: " << users[uid].honorScore << "\n";
            }

        } else if (ch == 4) {
            divider();
            cout << "  Enter user ID to delete: ";
            string uid; getline(cin, uid); uid = trim(uid);
            if (!users.count(uid) || users[uid].isAdmin) { cout << "  [!] Cannot delete.\n"; }
            else {
                string name = users[uid].name;
                // Remove all edges
                for (auto& fid : getFriendsList(uid))
                    removeFriendship(uid, fid);
                graph.erase(uid);
                nameToId.erase(name);
                users.erase(uid);
                saveUsers(); saveFriendships();
                cout << "  User " << name << " deleted.\n";
            }

        } else if (ch == 5) {
            divider();
            int totalEdges = 0;
            for (auto& kv : graph) totalEdges += kv.second.size();
            int n = (int)users.size() - 1; // exclude admin
            cout << "  Users         : " << n << "\n";
            cout << "  Friendships   : " << totalEdges / 2 << "\n";
            cout << "  Graph density : ";
            if (n > 1)
                cout << fixed << setprecision(4)
                     << (double)(totalEdges/2) / ((double)n*(n-1)/2.0) << "\n";
            else cout << "N/A\n";

        } else {
            cout << "  Admin logged out.\n";
            return;
        }
    }
}
