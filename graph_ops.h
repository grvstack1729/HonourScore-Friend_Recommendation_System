#pragma once
/*
 * ------------------------------------------------------------------------------------
 *  graph_ops.h  —  Graph Operations (DAA Core)
 *
 *  The social network is represented as a
 *  weighted undirected graph (adjacency list).
 *
 *  DAA Concepts:
 *   - Graph ADT (adjacency list)
 *   - BFS for traversal
 *   - Weighted edges (interaction scores)
 * ------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>
using namespace std;

bool areFriends      (const string& userId1, const string& userId2);
void addFriendship   (const string& userId1, const string& userId2, double weight = 50.0);
void removeFriendship(const string& userId1, const string& userId2);
int  countMutualFriends(const string& userId1, const string& userId2);
vector<string> getFriendsList(const string& userId);
