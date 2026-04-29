/*
 * ------------------------------------------------------------------------------------
 *  honor.cpp  —  Honor Score Engine
 * ------------------------------------------------------------------------------------
 *
 *  Score change events:
 *   +1   user logs in
 *   +1   user requests recommendations
 *   +1   user sends a friend request
 *   +2   user accepts a friend request
 *   +3   both sides when friendship is formed
 *   -5   admin manual penalty
 *   any  admin arbitrary adjustment
 *
 *  Cold start: new user gets 60-70 so they appear
 *  in recommendations but not at the very top.
 * ------------------------------------------------------------------------------------
 */

#include "honor.h"
#include "types.h"
#include <cstdlib>
#include <algorithm>

using namespace std;

int coldStartScore() {
    return 60 + rand() % 11; // 60..70
}

void adjustHonorScore(const string& userId, int delta, const string& /*reason*/) {
    if (!users.count(userId)) return;
    users[userId].honorScore = max(0, min(100, users[userId].honorScore + delta));
    // reason is intentionally unused in output — score changes are silent
}

bool shouldWarnUser(const string& userId) {
    if (!users.count(userId)) return false;
    return users[userId].honorScore < 40;
}
