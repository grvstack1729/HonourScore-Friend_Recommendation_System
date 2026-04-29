/*
 * ------------------------------------------------------------------------------------
 *  graph_ops.cpp  —  Graph Operations
 * ------------------------------------------------------------------------------------
 */

#include "graph_ops.h"
#include "types.h"
#include "honor.h"
#include "storage.h"
#include <algorithm>
#include <set>

using namespace std;

bool areFriends(const string& uid1, const string& uid2) {
    if (!graph.count(uid1)) return false;
    for (auto& e : graph[uid1])
        if (e.first == uid2) return true;
    return false;
}

void addFriendship(const string& uid1, const string& uid2, double weight) {
    if (areFriends(uid1, uid2)) return;
    graph[uid1].push_back({uid2, weight});
    graph[uid2].push_back({uid1, weight});
    saveFriendships();
    // Both users gain honor for forming a valid connection
    adjustHonorScore(uid1, +3, "friendship formed");
    adjustHonorScore(uid2, +3, "friendship formed");
    saveUsers();
}

void removeFriendship(const string& uid1, const string& uid2) {
    auto removeEdge = [&](const string& a, const string& b) {
        auto& vec = graph[a];
        vec.erase(remove_if(vec.begin(), vec.end(),
            [&](const pair<string,double>& p){ return p.first == b; }),
            vec.end());
    };
    removeEdge(uid1, uid2);
    removeEdge(uid2, uid1);
    saveFriendships();
}

int countMutualFriends(const string& uid1, const string& uid2) {
    if (!graph.count(uid1) || !graph.count(uid2)) return 0;
    set<string> f1;
    for (auto& e : graph[uid1]) f1.insert(e.first);
    int cnt = 0;
    for (auto& e : graph[uid2])
        if (f1.count(e.first)) ++cnt;
    return cnt;
}

vector<string> getFriendsList(const string& userId) {
    set<string> unique;
    vector<string> result;

    if (!graph.count(userId)) return result;

    for (auto& e : graph[userId]) {
        if (!unique.count(e.first)) {
            unique.insert(e.first);
            result.push_back(e.first);
        }
    }
    return result;
}
 