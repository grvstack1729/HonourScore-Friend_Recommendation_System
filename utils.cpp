/*
 * ------------------------------------------------------------------------------------
 *  utils.cpp  —  Utility implementations
 * ------------------------------------------------------------------------------------
 */

#include "utils.h"
#include "types.h"
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

// Trim leading/trailing whitespace
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Case-insensitive string comparison
bool iequal(const string& a, const string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (tolower(a[i]) != tolower(b[i])) return false;
    return true;
}

// Safe bounded integer input with validation loop
int safeIntInput(const string& prompt, int lo, int hi) {
    int val;
    while (true) {
        if (!prompt.empty()) cout << prompt;
        if (cin >> val && val >= lo && val <= hi) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return val;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  [!] Enter a number between " << lo << " and " << hi << ".\n";
    }
}

// Print a divider line
void divider(char c, int len) {
    cout << string(len, c) << "\n";
}

// Generate next sequential user ID like U1001, U1002 ...
// Scans existing IDs and returns max+1
string generateUserId() {
    int maxId = 1000;
    for (auto& kv : users) {
        const string& id = kv.first;
        if (id.size() > 1 && id[0] == 'U') {
            try {
                int n = stoi(id.substr(1));
                if (n > maxId) maxId = n;
            } catch (...) {}
        }
    }
    return "U" + to_string(maxId + 1);
}
