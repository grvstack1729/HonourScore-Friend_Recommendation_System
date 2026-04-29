/*
 * ------------------------------------------------------------------------------------
 *  reports.cpp  —  User Report Module
 * ------------------------------------------------------------------------------------
 *
 *  Logic:
 *   - reportUser() validates the request, deducts
 *     -5 honor from the reported user, and logs it.
 *   - One report per (reporter, target) pair — no spam.
 *   - Reports persist in reports.csv across runs.
 *
 *  reports.csv columns:
 *    reportedUserId, reportedByUserId, reason, timestamp
 * ------------------------------------------------------------------------------------
 */

#include "Reports.h"
#include "types.h"
#include "honor.h"
#include "storage.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>

using namespace std;

vector<Report> allReports;

static const string REPORTS_FILE = "reports.csv";

// return current timestamp string
static string nowString() {
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
    return string(buf);
}

// File a report
string reportUser(const string& reporterId, const string& targetId, const string& reason) {
    // Validate
    if (reporterId == targetId)
        return "You cannot report yourself.";
    if (!users.count(targetId))
        return "Target user not found.";
    if (users[targetId].isAdmin)
        return "Admin accounts cannot be reported.";
    if (!users.count(reporterId))
        return "Reporter not found.";

    // One report per pair — check for duplicate
    for (auto& r : allReports) {
        if (r.reportedByUserId == reporterId && r.reportedUserId == targetId)
            return "You have already reported this user.";
    }

    // Log the report
    Report rpt;
    rpt.reportedUserId   = targetId;
    rpt.reportedByUserId = reporterId;
    rpt.reason           = reason.empty() ? "No reason given" : reason;
    rpt.timestamp        = nowString();
    allReports.push_back(rpt);

    // Deduct honor from target (-5 per report)
    adjustHonorScore(targetId, -5, "reported by user");
    saveUsers();   // flush honor change

    saveReports();
    return "";     // empty = success
}

// Count reports received by a user 
int getReportCount(const string& userId) {
    int cnt = 0;
    for (auto& r : allReports)
        if (r.reportedUserId == userId) ++cnt;
    return cnt;
}

// Accessor 
const vector<Report>& getReports() {
    return allReports;
}

// Persistence: save 
void saveReports() {
    ofstream f(REPORTS_FILE);
    f << "reportedUserId,reportedByUserId,reason,timestamp\n";
    for (auto& r : allReports) {
        // Sanitise commas inside fields by wrapping in quotes
        f << r.reportedUserId   << ","
          << r.reportedByUserId << ","
          << "\"" << r.reason   << "\","
          << r.timestamp        << "\n";
    }
}

// Persistence: load 
void loadReports() {
    allReports.clear();
    ifstream f(REPORTS_FILE);
    if (!f.is_open()) return;

    string line;
    getline(f, line); // skip header

    while (getline(f, line)) {
        if (trim(line).empty()) continue;
        // Simple CSV parse — handles one quoted field (reason)
        stringstream ss(line);
        Report r;
        string tok;

        getline(ss, r.reportedUserId,   ',');
        getline(ss, r.reportedByUserId, ',');

        // reason may be quoted
        getline(ss, tok, ',');
        if (!tok.empty() && tok.front() == '"') {
            // consume until closing quote
            string rest;
            while (tok.back() != '"' && getline(ss, rest, ','))
                tok += "," + rest;
            // strip quotes
            r.reason = tok.substr(1, tok.size() - 2);
        } else {
            r.reason = tok;
        }

        getline(ss, r.timestamp);

        r.reportedUserId   = trim(r.reportedUserId);
        r.reportedByUserId = trim(r.reportedByUserId);
        r.timestamp        = trim(r.timestamp);

        if (!r.reportedUserId.empty())
            allReports.push_back(r);
    }
}
