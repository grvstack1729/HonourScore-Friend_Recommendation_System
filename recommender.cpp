/*
 * ------------------------------------------------------------------------------------
 *  recommender.cpp  —  Friend Recommendation Engine
 * ------------------------------------------------------------------------------------
 *
 *  Algorithm (step by step):
 *
 *  Step 1 — BFS from `userId` up to depth 2
 *    - depth-1 nodes  = direct friends       (excluded from results)
 *    - depth-2 nodes  = friends-of-friends   (main candidates)
 *    - BFS also builds a `graphScore` map:
 *        graphScore[v] += edge_weight / depth
 *
 *  Step 2 — Score each candidate (not a direct friend, not self)
 *    score = W_HONOR    * honorScore/100
 *          + W_MUTUAL   * min(mutualFriends/10, 1)
 *          + W_HOBBY    * sameHobby
 *          + W_AGE      * 1/(1+|ageDiff|)
 *          + W_INTEREST * sameInterest
 *          + W_PROF     * sameProfession
 *          + W_GRAPH    * graphScore/maxGraphScore
 *
 *  Step 3 — Push all scored candidates into a max-heap
 *
 *  Step 4 — Pop top-K from the heap
 *
 *  Time complexity: O(V + E) BFS  +  O(U log U) heap  =  O(U log U)
 *  Space complexity: O(V + E) for BFS visited maps
 * ------------------------------------------------------------------------------------
 */

#include "recommender.h"
#include "graph_ops.h"
#include "utils.h"
#include <queue>
#include <set>
#include <algorithm>
#include <cmath>

using namespace std;

vector<Candidate> recommendTopK(const string& userId, int K) {
    if (!users.count(userId)) return {};
    const User& me = users[userId];

    // Step 1: BFS ─
    unordered_map<string, double> graphScore;
    unordered_map<string, int>    visited;
    set<string>                   directFriends;
    queue<pair<string,int>>       bfsQ;

    if (graph.count(userId)) {
        for (auto& e : graph[userId]) {
            directFriends.insert(e.first);
            if (!visited[e.first]) {
                visited[e.first] = 1;
                graphScore[e.first] += e.second;
                bfsQ.push({e.first, 1});
            }
        }
    }

    while (!bfsQ.empty()) {
        pair<string, int> front = bfsQ.front();
        bfsQ.pop();
        string cur = front.first;
        int level = front.second;
        if (level >= 2) continue;
        if (!graph.count(cur)) continue;
        for (auto& e : graph[cur]) {
            if (e.first == userId) continue;
            if (!visited[e.first] || visited[e.first] > level + 1) {
                visited[e.first] = level + 1;
                graphScore[e.first] += e.second / (level + 1.0);
                bfsQ.push({e.first, level + 1});
            }
        }
    }

    // Normalise graphScore
    double maxGS = 1.0;
    for (auto& kv : graphScore) maxGS = max(maxGS, kv.second);

    // Step 2 & 3: Score + Max-Heap 
    priority_queue<Candidate> pq;

    for (auto& kv : users) {
        const string& cid  = kv.first;
        const User&   cand = kv.second;

        if (cid == userId)          continue;  // skip self
        if (cand.isAdmin)           continue;  // skip admin accounts
        if (directFriends.count(cid)) continue; // skip existing friends

        double score = 0.0;

        // 1. Honor score (normalised 0→1)
        score += W_HONOR * (cand.honorScore / 100.0);

        // 2. Mutual friends (normalised, cap at 10)
        int mutual = countMutualFriends(userId, cid);
        score += W_MUTUAL * min(mutual / 10.0, 1.0);

        // 3. Shared hobby
        if (iequal(me.hobby, cand.hobby)) score += W_HOBBY;

        // 4. Age proximity — smooth decay: 1 / (1 + |diff|)
        score += W_AGE * (1.0 / (1.0 + abs(me.age - cand.age)));

        // 5. Shared interest
        if (iequal(me.interest, cand.interest)) score += W_INTEREST;

        // 6. Shared profession
        if (iequal(me.profession, cand.profession)) score += W_PROF;

        // 7. Graph proximity (from BFS)
        if (graphScore.count(cid))
            score += W_GRAPH * (graphScore[cid] / maxGS);

        pq.push({cand.name, cid, score});
    }

    // Step 4: Pop top-K ──
    vector<Candidate> result;
    while (!pq.empty() && (int)result.size() < K) {
        result.push_back(pq.top());
        pq.pop();
    }
    return result;
}
