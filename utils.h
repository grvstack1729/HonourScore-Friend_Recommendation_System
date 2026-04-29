#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  utils.h  —  Utility function declarations
 * ------------------------------------------------------------------------------------
 */

#include <string>
#include <limits>
using namespace std;

string trim(const string& s);
bool   iequal(const string& a, const string& b);
int    safeIntInput(const string& prompt, int lo = 0, int hi = 9999);
void   divider(char c = '-', int len = 55);
string generateUserId();   // returns next unique "U1001", "U1002", …
