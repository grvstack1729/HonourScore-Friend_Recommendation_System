#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  types.h  —  Shared data structures & constants
 *  Used by every module in the system.
 * ------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <set>

using namespace std;

// User Profile ─
struct User {
    string userId;      // Unique ID  e.g. "U1001"
    string name;
    int    age;
    string school;
    string hobby;
    string profession;
    string interest;
    int    honorScore;  // Hidden from user; visible only to admin & engine
    bool   isAdmin;
    string password;
};

// Candidate for recommendation ────
struct Candidate {
    string name;
    string userId;
    double score;
    bool operator<(const Candidate& o) const { return score < o.score; }  // max-heap
};

// Global data stores ──────
extern unordered_map<string, User>                           users;       // userId  -> User
extern unordered_map<string, string>                         nameToId;    // name    -> userId
extern unordered_map<string, vector<pair<string, double>>>   graph;       // userId  -> [(friendId, weight)]
extern unordered_map<string, set<string>>                    pendingReqs; // senderUserId -> {receiverUserIds}

// File paths ─────
extern const string USERS_FILE;
extern const string FRIENDSHIPS_FILE;
extern const string INTERACTIONS_FILE;

// Recommendation weights ──
constexpr double W_HONOR    = 0.30;
constexpr double W_MUTUAL   = 0.25;
constexpr double W_HOBBY    = 0.15;
constexpr double W_AGE      = 0.10;
constexpr double W_INTEREST = 0.10;
constexpr double W_PROF     = 0.05;
constexpr double W_GRAPH    = 0.05;
