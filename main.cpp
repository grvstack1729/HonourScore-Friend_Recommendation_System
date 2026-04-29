/*
 * ------------------------------------------------------------------------------------
 *  main.cpp  —  Entry Point
 *
 *  Friend Recommendation System — DAA Project
 *
 *  Module structure:
 *    types.h        shared structs & globals
 *    utils           trim, input helpers, ID gen
 *    storage         CSV read/write
 *    honor           honor score engine
 *    graph_ops       adjacency list operations
 *    recommender     BFS + max-heap engine
 *    auth            login / register
 *    menus           terminal UI
 * ------------------------------------------------------------------------------------
 */

#include "types.h"
#include "utils.h"
#include "storage.h"
#include "honor.h"
#include "auth.h"
#include "menus.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int main() {
    srand((unsigned)time(nullptr));

    // Load all data from CSV files ─
    loadUsers();
    loadFriendships();

    // Seed default admin if none exists ─────
    if (!users.count("ADMIN")) {
        User admin;
        admin.userId     = "ADMIN";
        admin.name       = "Administrator";
        admin.age        = 0;
        admin.school     = "";
        admin.hobby      = "";
        admin.profession = "";
        admin.interest   = "";
        admin.honorScore = 100;
        admin.isAdmin    = true;
        admin.password   = "admin123";
        users["ADMIN"]   = admin;
        saveUsers();
    }

    // Main login loop ─────
    cout << "\n";
    divider('=', 55);
    cout << "     FRIEND RECOMMENDATION SYSTEM\n";
    cout << "     DAA Project\n";
    divider('=', 55);

    while (true) {
        cout << "\n  1. Login\n";
        cout << "  2. Register\n";
        cout << "  3. Exit\n";
        divider();

        int ch = safeIntInput("  Choice: ", 1, 3);

        if (ch == 1) {
            cout << "  User ID  : ";
            string uid; getline(cin, uid); uid = trim(uid);
            cout << "  Password : ";
            string pwd; getline(cin, pwd); pwd = trim(pwd);

            string result = loginUser(uid, pwd);
            if (result.empty()) {
                cout << "  [!] Invalid credentials.\n";
                continue;
            }

            if (users[result].isAdmin) {
                adminMenu();
            } else {
                adjustHonorScore(result, +1, "login");
                saveUsers();
                userMenu(result);
            }

        } else if (ch == 2) {
            string newId = registerUser();
            if (!newId.empty()) {
                cout << "  You can now login with your User ID: " << newId << "\n";
            }

        } else {
            cout << "\n  Goodbye!\n\n";
            break;
        }
    }
    return 0;
}
