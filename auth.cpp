/*
 * ------------------------------------------------------------------------------------
 *  auth.cpp  —  Authentication Module
 * ------------------------------------------------------------------------------------
 */

#include "auth.h"
#include "types.h"
#include "utils.h"
#include "honor.h"
#include "storage.h"
#include <iostream>

using namespace std;

string loginUser(const string& userId, const string& password) {
    auto it = users.find(userId);
    if (it == users.end()) return "";
    if (it->second.password != password) return "";
    return userId;
}

string registerUser() {
    cout << "\n Register New Account\n\n";
    User u;
    u.isAdmin = false;

    cout << "  Full Name      : "; getline(cin, u.name); u.name = trim(u.name);
    if (u.name.empty()) {
        cout << "  [!] Name cannot be empty.\n"; return "";
    }

    // Check name uniqueness (names should also be distinct for usability)
    if (nameToId.count(u.name)) {
        cout << "  [!] A user with that name already exists.\n"; return "";
    }

    u.age        = safeIntInput("  Age            : ", 5, 120);
    cout << "  School/College : "; getline(cin, u.school);    u.school     = trim(u.school);
    cout << "  Hobby          : "; getline(cin, u.hobby);     u.hobby      = trim(u.hobby);
    cout << "  Profession     : "; getline(cin, u.profession);u.profession = trim(u.profession);
    cout << "  Interest       : "; getline(cin, u.interest);  u.interest   = trim(u.interest);
    cout << "  Password       : "; getline(cin, u.password);  u.password   = trim(u.password);
    if (u.password.empty()) {
        cout << "  [!] Password cannot be empty.\n"; return "";
    }

    u.userId     = generateUserId();
    u.honorScore = coldStartScore(); // 60–70, hidden

    users[u.userId]  = u;
    nameToId[u.name] = u.userId;
    saveUsers();

    cout << "\n  ✓ Account created!\n";
    cout << "  Your User ID is: " << u.userId << "  (save this for login)\n";
    return u.userId;
}
