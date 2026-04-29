#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  auth.h  —  Authentication Module
 *
 *  Handles login (by userId + password) and
 *  new user registration (generates unique userId).
 * ------------------------------------------------------------------------------------
 */

#include <string>
using namespace std;

// Returns userId on success, "" on failure
string loginUser(const string& userId, const string& password);

// Returns new userId on success, "" on failure
string registerUser();
