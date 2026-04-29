#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  honor.h  —  Honor Score Engine
 *
 *  Honor score is INVISIBLE to regular users.
 *  It is only seen by admin and used by the
 *  recommendation engine internally.
 *
 *  Score range : 0 – 100
 *  Cold start  : 60 – 70  (random, for new users)
 *  User warning: shown only when score < 40
 * ------------------------------------------------------------------------------------
 */

#include <string>
using namespace std;

int  coldStartScore();
void adjustHonorScore(const string& userId, int delta, const string& reason = "");
bool shouldWarnUser(const string& userId);
