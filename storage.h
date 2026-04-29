#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  storage.h  —  CSV persistence layer
 *
 *  All reads/writes to disk go through this module.
 *  Data is loaded into global maps at startup and
 *  flushed back after every mutating operation.
 * ------------------------------------------------------------------------------------
 */

#include <string>
using namespace std;

void loadUsers();
void saveUsers();

void loadFriendships();
void saveFriendships();

void loadInteractionsCSV(const string& filename);
