/*
 * ------------------------------------------------------------------------------------
 *  server.cpp  —  HTTP REST API Server
 *
 *  Friend Recommendation System — DAA Project
 *  GUI Backend using cpp-httplib (single-header)
 *
 *  All existing modules (auth, graph_ops, honor,
 *  recommender, storage, reports) are reused as-is.
 *  This file is the only new "glue" needed.
 *
 *  PORT: 8080  (change below if needed)
 *  Run: ./server   then open index.html in browser
 *
 *  API Endpoints:
 *    POST /api/login
 *    POST /api/register
 *    GET  /api/profile?userId=X
 *    GET  /api/friends?userId=X
 *    GET  /api/recommendations?userId=X
 *    GET  /api/search?q=X&userId=X
 *    GET  /api/requests?userId=X
 *    POST /api/send-request
 *    POST /api/accept-request
 *    POST /api/reject-request
 *    POST /api/report
 *    GET  /api/admin/users
 *    GET  /api/admin/stats
 *    POST /api/admin/adjust-score
 *    POST /api/admin/delete-user
 *    GET  /api/admin/reports
 * ------------------------------------------------------------------------------------
 */

#include "httplib.h"    // single-header HTTP library
#include "types.h"
#include "utils.h"
#include "storage.h"
#include "honor.h"
#include "auth.h"
#include "graph_ops.h"
#include "recommender.h"
#include "reports.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;
using namespace httplib;

// JSON helpers
// Minimal hand-rolled JSON — no external lib needed

static string jsonString(const string& s) {
    // Escape backslash and double-quote for JSON safety
    string out = "\"";
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    out += "\"";
    return out;
}

static string userToJson(const User& u, bool showHonor = false) {
    ostringstream j;
    j << "{"
      << "\"userId\":"     << jsonString(u.userId)     << ","
      << "\"name\":"       << jsonString(u.name)       << ","
      << "\"age\":"        << u.age                    << ","
      << "\"school\":"     << jsonString(u.school)     << ","
      << "\"hobby\":"      << jsonString(u.hobby)      << ","
      << "\"profession\":" << jsonString(u.profession) << ","
      << "\"interest\":"   << jsonString(u.interest)   << ","
      << "\"isAdmin\":"    << (u.isAdmin ? "true" : "false");
    if (showHonor)
        j << ",\"honorScore\":" << u.honorScore;
    // Always include reportCount (public info for UI)
    j << ",\"reportCount\":" << getReportCount(u.userId);
    j << "}";
    return j.str();
}

static string ok(const string& body) {
    return "{\"ok\":true," + body + "}";
}
static string err(const string& msg) {
    return "{\"ok\":false,\"error\":" + jsonString(msg) + "}";
}

// Parse a simple JSON body key (only works for flat string values)
static string parseJsonField(const string& body, const string& key) {
    // Look for  "key":"value"  or  "key": "value"
    string pat = "\"" + key + "\"";
    auto pos = body.find(pat);
    if (pos == string::npos) return "";
    pos += pat.size();
    // Skip whitespace and colon
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == ':' || body[pos] == '\t')) ++pos;
    if (pos >= body.size()) return "";
    if (body[pos] == '"') {
        // string value
        ++pos;
        string val;
        while (pos < body.size() && body[pos] != '"') {
            if (body[pos] == '\\' && pos+1 < body.size()) { ++pos; val += body[pos]; }
            else val += body[pos];
            ++pos;
        }
        return val;
    } else {
        // numeric / bool value
        string val;
        while (pos < body.size() && body[pos] != ',' && body[pos] != '}' && body[pos] != ' ')
            val += body[pos++];
        return val;
    }
}

// CORS headers (allow browser fetch from file://)
static void addCORS(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
    res.set_header("Content-Type", "application/json");
}

// ════
//  MAIN
// ════
int main() {
    srand((unsigned)time(nullptr));

    // Load data
    loadUsers();
    loadFriendships();
    loadReports();
    loadInteractionsCSV("FriendInteractions2.csv");

    // Seed admin if missing
    if (!users.count("ADMIN")) {
        User admin;
        admin.userId = "ADMIN"; admin.name = "Administrator";
        admin.age = 0; admin.honorScore = 100; admin.isAdmin = true;
        admin.password = "admin123";
        users["ADMIN"] = admin;
        saveUsers();
    }

    Server svr;

    // OPTIONS preflight (CORS)
    svr.Options(".*", [](const Request&, Response& res) {
        addCORS(res);
        res.status = 204;
    });

    //
    // POST /api/login
    // Body: { "userId": "U1001", "password": "arjun123" }
    //
    svr.Post("/api/login", [](const Request& req, Response& res) {
        addCORS(res);
        string uid = parseJsonField(req.body, "userId");
        string pwd = parseJsonField(req.body, "password");

        string result = loginUser(uid, pwd);
        if (result.empty()) {
            res.body = err("Invalid credentials");
            return;
        }

        bool isAdmin = users[result].isAdmin;
        if (!isAdmin) adjustHonorScore(result, +1, "login");
        saveUsers();

        // Return user profile (honor score only for admin viewing themselves)
        res.body = ok("\"userId\":" + jsonString(result)
                    + ",\"isAdmin\":" + (isAdmin ? "true" : "false")
                    + ",\"warn\":" + (shouldWarnUser(result) ? "true" : "false")
                    + ",\"user\":" + userToJson(users[result], isAdmin));
    });

    //
    // POST /api/register
    //
    svr.Post("/api/register", [](const Request& req, Response& res) {
        addCORS(res);
        string name       = parseJsonField(req.body, "name");
        string ageStr     = parseJsonField(req.body, "age");
        string school     = parseJsonField(req.body, "school");
        string hobby      = parseJsonField(req.body, "hobby");
        string profession = parseJsonField(req.body, "profession");
        string interest   = parseJsonField(req.body, "interest");
        string password   = parseJsonField(req.body, "password");

        if (name.empty())     { res.body = err("Name cannot be empty");     return; }
        if (password.empty()) { res.body = err("Password cannot be empty"); return; }

        int age = 0;
        try { age = stoi(ageStr); } catch (...) {}
        if (age < 5 || age > 120) { res.body = err("Age must be 5–120"); return; }

        // Name uniqueness check (mirrors auth.cpp)
        for (auto& kv : users) {
            if (!kv.second.isAdmin &&
                kv.second.name.size() == name.size() &&
                iequal(kv.second.name, name)) {
                res.body = err("A user with that name already exists");
                return;
            }
        }

        User u;
        u.userId     = generateUserId();
        u.name       = name;
        u.age        = age;
        u.school     = school;
        u.hobby      = hobby;
        u.profession = profession;
        u.interest   = interest;
        u.password   = password;
        u.isAdmin    = false;
        u.honorScore = coldStartScore();

        users[u.userId]  = u;
        nameToId[u.name] = u.userId;
        graph[u.userId]  = {};
        saveUsers();

        res.body = ok("\"userId\":" + jsonString(u.userId)
                    + ",\"message\":\"Account created! Your User ID is " + u.userId + "\"");
    });

    //
    // GET /api/profile?userId=X
    //
    svr.Get("/api/profile", [](const Request& req, Response& res) {
        addCORS(res);
        string uid = req.get_param_value("userId");
        if (!users.count(uid)) { res.body = err("User not found"); return; }
        bool showHonor = (req.get_param_value("admin") == "1");
        res.body = ok("\"user\":" + userToJson(users[uid], showHonor)
                    + ",\"warn\":" + (shouldWarnUser(uid) ? "true" : "false"));
    });

    //
    // GET /api/friends?userId=X
    //
    svr.Get("/api/friends", [](const Request& req, Response& res) {
        addCORS(res);
        string uid = req.get_param_value("userId");
        if (!users.count(uid)) { res.body = err("User not found"); return; }

        auto friends = getFriendsList(uid);
        ostringstream j;
        j << "[";
        bool first = true;
        for (auto& fid : friends) {
            if (!users.count(fid)) continue;
            if (!first) j << ",";
            j << userToJson(users[fid], false);
            first = false;
        }
        j << "]";
        res.body = ok("\"friends\":" + j.str());
    });

    //
    // GET /api/recommendations?userId=X
    //
    svr.Get("/api/recommendations", [](const Request& req, Response& res) {
        addCORS(res);
        string uid = req.get_param_value("userId");
        if (!users.count(uid)) { res.body = err("User not found"); return; }

        adjustHonorScore(uid, +1, "used recommendations");
        saveUsers();

        auto recs = recommendTopK(uid, 5);
        ostringstream j;
        j << "[";
        for (int i = 0; i < (int)recs.size(); ++i) {
            if (i) j << ",";
            const User& c = users[recs[i].userId];
            j << "{"
              << "\"userId\":"     << jsonString(recs[i].userId)   << ","
              << "\"name\":"       << jsonString(c.name)           << ","
              << "\"age\":"        << c.age                        << ","
              << "\"hobby\":"      << jsonString(c.hobby)          << ","
              << "\"profession\":" << jsonString(c.profession)     << ","
              << "\"interest\":"   << jsonString(c.interest)       << ","
              << "\"school\":"     << jsonString(c.school)         << ","
              << "\"score\":"      << fixed << setprecision(4) << recs[i].score
              << "}";
        }
        j << "]";
        res.body = ok("\"recommendations\":" + j.str());
    });

    //
    // GET /api/search?q=X&userId=X
    //
    svr.Get("/api/search", [](const Request& req, Response& res) {
        addCORS(res);
        string q   = req.get_param_value("q");
        string uid = req.get_param_value("userId");

        // lowercase query
        string ql = q;
        for (char& c : ql) c = tolower(c);

        ostringstream j;
        j << "[";
        bool first = true;
        for (auto& kv : users) {
            const User& u = kv.second;
            if (u.isAdmin || u.userId == uid) continue;

            string nameLow = u.name;
            string idLow   = u.userId;
            for (char& c : nameLow) c = tolower(c);
            for (char& c : idLow)   c = tolower(c);

            if (nameLow.find(ql) != string::npos || idLow.find(ql) != string::npos) {
                if (!first) j << ",";
                bool friendCheck = areFriends(uid, u.userId);
                string uj = userToJson(u, false);
                uj = uj.substr(0, uj.size()-1);
                j << uj
                  << ",\"isFriend\":" << (friendCheck ? "true" : "false")
                  << "}";
                first = false;
            }
        }
        j << "]";
        res.body = ok("\"results\":" + j.str());
    });

    //
    // GET /api/requests?userId=X
    // Returns incoming pending requests
    //
    svr.Get("/api/requests", [](const Request& req, Response& res) {
        addCORS(res);
        string uid = req.get_param_value("userId");

        ostringstream j;
        j << "[";
        bool first = true;
        for (auto& kv : pendingReqs) {
            if (kv.second.count(uid) && users.count(kv.first)) {
                if (!first) j << ",";
                j << userToJson(users[kv.first], false);
                first = false;
            }
        }
        j << "]";
        res.body = ok("\"requests\":" + j.str());
    });

    //
    // POST /api/send-request
    // Body: { "fromId": "U1001", "toId": "U1005" }
    //
    svr.Post("/api/send-request", [](const Request& req, Response& res) {
        addCORS(res);
        string from = parseJsonField(req.body, "fromId");
        string to   = parseJsonField(req.body, "toId");

        if (!users.count(to))            { res.body = err("User not found");                  return; }
        if (from == to)                   { res.body = err("Cannot send request to yourself"); return; }
        if (areFriends(from, to))         { res.body = err("Already friends");                 return; }
        if (pendingReqs[from].count(to))  { res.body = err("Request already sent");            return; }

        pendingReqs[from].insert(to);
        adjustHonorScore(from, +1, "sent friend request");
        saveUsers();

        res.body = ok("\"message\":\"Request sent to " + users[to].name + "\"");
    });

    //
    // POST /api/accept-request
    // Body: { "userId": "U1002", "senderId": "U1003" }
    //
    svr.Post("/api/accept-request", [](const Request& req, Response& res) {
        addCORS(res);
        string uid    = parseJsonField(req.body, "userId");
        string sender = parseJsonField(req.body, "senderId");

        addFriendship(uid, sender);
        pendingReqs[sender].erase(uid);
        adjustHonorScore(uid, +2, "accepted request");
        saveUsers();

        res.body = ok("\"message\":\"You are now friends with " + users[sender].name + "\"");
    });

    //
    // POST /api/reject-request
    // Body: { "userId": "U1002", "senderId": "U1003" }
    //
    svr.Post("/api/reject-request", [](const Request& req, Response& res) {
        addCORS(res);
        string uid    = parseJsonField(req.body, "userId");
        string sender = parseJsonField(req.body, "senderId");

        pendingReqs[sender].erase(uid);
        res.body = ok("\"message\":\"Request rejected\"");
    });

    //
    // POST /api/report
    // Body: { "reporterId": "U1001", "targetId": "U1005", "reason": "Spam" }
    //
    svr.Post("/api/report", [](const Request& req, Response& res) {
        addCORS(res);
        string reporter = parseJsonField(req.body, "reporterId");
        string target   = parseJsonField(req.body, "targetId");
        string reason   = parseJsonField(req.body, "reason");

        string errMsg = reportUser(reporter, target, reason);
        if (!errMsg.empty()) {
            res.body = err(errMsg);
            return;
        }
        res.body = ok("\"message\":\"Report filed. Thank you for keeping the community safe.\"");
    });

    //
    // GET /api/admin/users
    // Returns all non-admin users sorted by honor score (max-heap order)
    //
    svr.Get("/api/admin/users", [](const Request&, Response& res) {
        addCORS(res);
        // Collect and sort descending by honor score (mirrors priority_queue in menus.cpp)
        vector<const User*> all;
        for (auto& kv : users)
            if (!kv.second.isAdmin)
                all.push_back(&kv.second);
        sort(all.begin(), all.end(), [](const User* a, const User* b){
            return a->honorScore > b->honorScore;
        });

        ostringstream j;
        j << "[";
        for (int i = 0; i < (int)all.size(); ++i) {
            if (i) j << ",";
            const User* u = all[i];
            string uj = userToJson(*u, true);
            uj = uj.substr(0, uj.size()-1); // strip closing }
            j << uj
              << ",\"friendCount\":" << getFriendsList(u->userId).size()
              << ",\"reportCount\":" << getReportCount(u->userId)
              << "}";
        }
        j << "]";
        res.body = ok("\"users\":" + j.str());
    });

    //
    // GET /api/admin/stats
    //
    svr.Get("/api/admin/stats", [](const Request&, Response& res) {
        addCORS(res);
        int n = 0;
        for (auto& kv : users) if (!kv.second.isAdmin) ++n;

        // Count unique edges
        set<pair<string,string>> seen;
        int totalEdges = 0;
        for (auto& kv : graph) {
            for (auto& e : kv.second) {
                string a = kv.first, b = e.first;
                if (a > b) swap(a, b);
                if (!seen.count({a, b})) { seen.insert({a,b}); ++totalEdges; }
            }
        }

        double density = (n > 1) ? (double)totalEdges / ((double)n*(n-1)/2.0) : 0;
        double avgConn = (n > 0) ? (totalEdges * 2.0) / n : 0;

        int pendingCount = 0;
        for (auto& kv : pendingReqs) pendingCount += kv.second.size();

        int isolated = 0;
        for (auto& kv : users)
            if (!kv.second.isAdmin && getFriendsList(kv.first).empty()) ++isolated;

        ostringstream j;
        j << "\"userCount\":"    << n
          << ",\"friendships\":" << totalEdges
          << ",\"density\":"     << fixed << setprecision(4) << density
          << ",\"avgConn\":"     << fixed << setprecision(2) << avgConn
          << ",\"pending\":"     << pendingCount
          << ",\"isolated\":"    << isolated;

        res.body = ok(j.str());
    });

    //
    // POST /api/admin/adjust-score
    // Body: { "userId": "U1001", "delta": -5 }
    //
    svr.Post("/api/admin/adjust-score", [](const Request& req, Response& res) {
        addCORS(res);
        string uid  = parseJsonField(req.body, "userId");
        string dStr = parseJsonField(req.body, "delta");

        if (!users.count(uid) || users[uid].isAdmin) {
            res.body = err("User not found"); return;
        }
        int delta = 0;
        try { delta = stoi(dStr); } catch (...) {
            res.body = err("Invalid delta"); return;
        }
        if (delta < -100 || delta > 100) {
            res.body = err("Delta must be between -100 and +100"); return;
        }

        int oldScore = users[uid].honorScore;
        adjustHonorScore(uid, delta, "admin adjustment");
        saveUsers();
        int newScore = users[uid].honorScore;

        res.body = ok("\"oldScore\":" + to_string(oldScore)
                    + ",\"newScore\":" + to_string(newScore)
                    + ",\"name\":"     + jsonString(users[uid].name));
    });

    //
    // POST /api/admin/delete-user
    // Body: { "userId": "U1001" }
    //
    svr.Post("/api/admin/delete-user", [](const Request& req, Response& res) {
        addCORS(res);
        string uid = parseJsonField(req.body, "userId");

        if (!users.count(uid) || users[uid].isAdmin) {
            res.body = err("Cannot delete: user not found or is admin"); return;
        }

        string name = users[uid].name;
        for (auto& fid : getFriendsList(uid)) removeFriendship(uid, fid);
        graph.erase(uid);
        nameToId.erase(name);
        users.erase(uid);

        // Clean pending requests
        for (auto& kv : pendingReqs) kv.second.erase(uid);
        pendingReqs.erase(uid);

        saveUsers();
        saveFriendships();

        res.body = ok("\"message\":\"User " + name + " deleted\"");
    });

    //
    // GET /api/admin/reports
    // Returns all filed reports (admin only)
    //
    svr.Get("/api/admin/reports", [](const Request&, Response& res) {
        addCORS(res);
        const auto& rpts = getReports();
        ostringstream j;
        j << "[";
        for (int i = 0; i < (int)rpts.size(); ++i) {
            if (i) j << ",";
            const Report& r = rpts[i];
            j << "{"
              << "\"reportedUserId\":"   << jsonString(r.reportedUserId)   << ","
              << "\"reportedByUserId\":" << jsonString(r.reportedByUserId) << ","
              << "\"reason\":"           << jsonString(r.reason)           << ","
              << "\"timestamp\":"        << jsonString(r.timestamp);

            // Enrich with names if users still exist
            if (users.count(r.reportedUserId))
                j << ",\"reportedName\":"   << jsonString(users[r.reportedUserId].name);
            if (users.count(r.reportedByUserId))
                j << ",\"reporterName\":"   << jsonString(users[r.reportedByUserId].name);

            j << "}";
        }
        j << "]";
        res.body = ok("\"reports\":" + j.str());
    });

    //
    // Start server
    //
    cout << "\n";
    cout << "\n";
    cout << "  Friend Recommendation System — DAA\n";
    cout << "  HTTP API Server running on port 8080\n";
    cout << "  Open index.html in your browser\n";
    cout << "\n\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}