#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  recommender.h  —  Friend Recommendation Engine
 *
 *  DAA Concepts:
 *   - BFS (breadth-first search) to depth 2
 *   - Max-Heap (priority_queue) for top-K selection
 *   - Composite scoring with weighted criteria
 *
 *  Priority order (encoded in weights):
 *   1. Honor score       (W_HONOR    = 0.30)
 *   2. Mutual friends    (W_MUTUAL   = 0.25)
 *   3. Shared hobby      (W_HOBBY    = 0.15)
 *   4. Age proximity     (W_AGE      = 0.10)
 *   5. Shared interest   (W_INTEREST = 0.10)
 *   6. Shared profession (W_PROF     = 0.05)
 *   7. Graph proximity   (W_GRAPH    = 0.05)
 * ------------------------------------------------------------------------------------
 */

#include "types.h"
#include <vector>
#include <string>
using namespace std;

// Returns top-K candidates with their scores
vector<Candidate> recommendTopK(const string& userId, int K = 5);
