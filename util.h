#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <queue>
#include <string>
#include <map>
#include <stack>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cfloat>
#include <limits>
#include <filesystem>
#include "graphics.h"

# define ll double
# define SOUTH 0
# define EAST 1
# define NORTH 2
# define WEST 3

using namespace std;

class PinLoc {
public:
    char b;
    int x, y, p;
    PinLoc() {
        b = 'u';
        x = -1;
        y = -1;
        p = -1;
    }
    PinLoc(const PinLoc& other) {
        b = other.b;
        x = other.x;
        y = other.y;
        p = other.p;
    }
    PinLoc(char _b, int _x, int _y, int _p) {
        b = _b;
        x = _x;
        y = _y;
        p = _p;
    }
    string toStr() {
        string strB = ""; strB.push_back(b);
        return to_string(x) + "_" + to_string(y) + "_" + strB + "_" + to_string(p);
    }
};
PinLoc str2Loc(string str) {
    vector<string> tokens;
    istringstream tokenStream(str);
    string token;
    while (getline(tokenStream, token, '_')) {
        tokens.push_back(token);
    }
    char b = tokens[2][0];
    int x = stoi(tokens[0]);
    int y = stoi(tokens[1]);
    int p = stoi(tokens[3]);
    return PinLoc(b, x, y, p);
}

class Segment {
public:
    char orient;
    int l;
    int r;
    int c;
    Segment() {
        orient = 'u';
        l = 0;
        r = 0;
        c = 0;
    };
    Segment(const Segment& other) {
        orient = other.orient;
        l = other.l;
        r = other.r;
        c = other.c;
    }
    Segment(char _orient, int _l, int _r, int _c) {
        orient = _orient;
        l = _l;
        r = _r;
        c = _c;
    };
    bool operator==(const Segment& other) const {
        return orient == other.orient && l == other.l && r == other.r && c == other.c;
    }
    bool operator-=(const Segment& other) const {
        return r == other.r && c == other.c && orient == other.orient;
    }
    bool operator!=(const Segment& other) const {
        return !(orient == other.orient && l == other.l && r == other.r && c == other.c);
    }
};

class PathNode {

public:

    // Pin / Segment
    bool isPin;
    PinLoc pin;
    bool isSegment;
    Segment segment;

    // Cost
    ll totalCost;

    // Tree
    bool isLocated; // Set to true when the node is located in a path through back trace
    PathNode* parent; // For back tracking
    vector<PathNode*> children; // Consider multi-load paths

    PathNode(bool _isPin, PinLoc _pin, bool _isSegment, Segment _segment, ll _totalCost) {
        isPin = _isPin;
        pin = _pin;
        isSegment = _isSegment;
        segment = _segment;
        totalCost = _totalCost;
        isLocated = 0;
        parent = NULL;
        children = {};
    }
};
void deleteTree(PathNode* cur) {
    for (PathNode* child : cur->children) {
        deleteTree(child);
    }
    free(cur);
}

#endif