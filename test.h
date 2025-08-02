#ifndef TEST_H
#define TEST_H

#include "util.h"

using namespace std;

class Test {
public:
    bool isDense;
    int N;
    int W;
    vector<PinLoc> srcs, sinks;
    Test() {
        isDense = false;
        N = 0;
        W = 0;
        srcs = {};
        sinks = {};
    }
    Test(const Test& other) {
        isDense = other.isDense;
        N = other.N;
        W = other.W;
        srcs = other.srcs;
        sinks = other.sinks;
    }
    Test(bool _isDense, int _N, int _W, vector<PinLoc> _srcs, vector<PinLoc> _sinks) {
        isDense = _isDense;
        N = _N;
        W = _W;
        srcs = _srcs;
        sinks = _sinks;
    }
};

Test genTest(string filePath) {
    bool isDense = filePath.find("dense") != string::npos;
    int N, W;
    vector<PinLoc> srcs, sinks;
    ifstream file(filePath);
    string line;
    // get N, W
    getline(file, line);
    N = stoi(line);
    getline(file, line);
    W = stoi(line);
    // get pins & loads
    while (1) {
        if (isDense) {
            int x0, y0, p0, x1, y1, p1;
            char b0, b1;
            file >> x0;
            file >> y0;
            file >> b0;
            file >> p0;
            file >> x1;
            file >> y1;
            file >> b1;
            file >> p1;
            if (x0 == -1) {
                break;
            }
            srcs.push_back(PinLoc(b0, x0, y0, p0));
            sinks.push_back(PinLoc(b1, x1, y1, p1));
        }
        else {
            int x0, y0, p0, x1, y1, p1;
            file >> x0;
            file >> y0;
            file >> p0;
            file >> x1;
            file >> y1;
            file >> p1;
            if (x0 == -1) {
                break;
            }
            srcs.push_back(PinLoc('a', x0, y0, p0));
            sinks.push_back(PinLoc('a', x1, y1, p1));
        }
    }
    file.close();
    return Test(isDense, N, W, srcs, sinks);
}

#endif