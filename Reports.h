#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  reports.h  —  User Report Module
 *
 *  Handles reporting of users by other users.
 *  - One report per (reporter, target) pair
 *  - Each report deducts -5 honor from target
 *  - Reports are logged with timestamp + reason
 *  - Admin can view all filed reports
 *  - Cannot report yourself or admin accounts
 * ------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>
using namespace std;

// A single filed report
struct Report {
    string reportedUserId;
    string reportedByUserId;
    string reason;
    string timestamp;  // ISO-8601 string  e.g. "2024-01-15 14:32"
};

// reports[targetUserId] = list of reports filed against them
extern vector<Report> allReports;

// File a report. Returns "" on success, error message on failure.
string reportUser(const string& reporterId, const string& targetId, const string& reason);

// How many reports has a user received?
int getReportCount(const string& userId);

// Get all reports (for admin view)
const vector<Report>& getReports();

// Persistence
void saveReports();
void loadReports();