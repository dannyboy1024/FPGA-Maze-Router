#ifndef SOLVER_H
#define SOLVER_H

#include "test.h"

using namespace std;

class Solver {

private:

    int overuse(Segment seg) {
        int use = useMatrix[seg.orient][seg.l][seg.r][seg.c];
        return max(0, use - 1);
    }

public:

    bool isDense;
    int N, W;
    int maxIterationNum;
    vector<int> numOverUsedSegsHistory;
    bool noOverUse;
    ll hFac, pFac;
    vector<vector<vector<vector<Segment>>>> layout;
    map<char, vector<vector<vector<ll>>>> costMatrix;
    map<char, vector<vector<vector<int>>>> useMatrix;
    map<char, vector<vector<vector<ll>>>> historyMatrix;
    map<char, vector<vector<vector<vector<bool>>>>> singleNetUseMatrix;
    map<string, vector<PinLoc>> src2sinks;
    vector<pair<string, vector<PinLoc>>> sortedSrc2sinks;
    vector<PathNode*> routes;

    // Initialize the layout, compute the src2sink map
    Solver(Test test) {

        // Get test data
        isDense = test.isDense;
        N = test.N;
        W = test.W;
        maxIterationNum = 60;
        numOverUsedSegsHistory = {};
        noOverUse = true;
        hFac = 1;
        pFac = 1;
        vector<PinLoc> srcs = test.srcs;
        vector<PinLoc> sinks = test.sinks;

        // Initialize routes
        routes = {};

        // Get Number of Switch Blocks per row/column
        int numSB = N + 1;

        // Initialize the layout by appending segments to each switch block
        // following the order: South -> East -> North -> West
        layout.resize(W, vector<vector<vector<Segment>>>(
            numSB, vector<vector<Segment>>(
                numSB, vector<Segment>(4)
            )
        ));
        Segment unExistingSeg = Segment('u', -1, -1, -1);
        for (int l = 0; l < W; l++) {
            for (int i = 0; i < numSB; i++) {
                for (int j = 0; j < numSB; j++) {
                    // South
                    if (i < numSB - 1) {
                        layout[l][i][j][SOUTH] = Segment('v', l, i, j);
                    }
                    else {
                        layout[l][i][j][SOUTH] = unExistingSeg;
                    }
                    // East
                    if (j < numSB - 1) {
                        layout[l][i][j][EAST] = Segment('h', l, i, j);
                    }
                    else {
                        layout[l][i][j][EAST] = unExistingSeg;
                    }
                    // North
                    if (i > 0) {
                        layout[l][i][j][NORTH] = Segment('v', l, i - 1, j);
                    }
                    else {
                        layout[l][i][j][NORTH] = unExistingSeg;
                    }
                    // West
                    if (j > 0) {
                        layout[l][i][j][WEST] = Segment('h', l, i, j - 1);
                    }
                    else {
                        layout[l][i][j][WEST] = unExistingSeg;
                    }
                }
            }
        }

        // Initially no segment is ever used
        useMatrix['v'] = vector<vector<vector<int>>>(
            W, vector<vector<int>>(
                numSB - 1, vector<int>(
                    numSB, 0
                )
            )
        );
        useMatrix['h'] = vector<vector<vector<int>>>(
            W, vector<vector<int>>(
                numSB, vector<int>(
                    numSB - 1, 0
                )
            )
        );

        // Initialize history values to 1
        historyMatrix['v'] = vector<vector<vector<ll>>>(
            W, vector<vector<ll>>(
                numSB - 1, vector<ll>(
                    numSB, 1
                )
            )
        );
        historyMatrix['h'] = vector<vector<vector<ll>>>(
            W, vector<vector<ll>>(
                numSB, vector<ll>(
                    numSB - 1, 1
                )
            )
        );

        // Initialize the cost for each segment
        costMatrix['v'] = vector<vector<vector<ll>>>(
            W, vector<vector<ll>>(
                numSB - 1, vector<ll>(
                    numSB, 1
                )
            )
        );
        costMatrix['h'] = vector<vector<vector<ll>>>(
            W, vector<vector<ll>>(
                numSB, vector<ll>(
                    numSB - 1, 1
                )
            )
        );

        // Condense source/sink pairs as there can be 1-to-N routings
        assert(srcs.size() == sinks.size());
        for (int i = 0; i < srcs.size(); i++) {
            string srcStr = srcs[i].toStr();
            src2sinks[srcStr].push_back(sinks[i]);
        }

        // Single net usage
        singleNetUseMatrix['v'] = vector<vector<vector<vector<bool>>>>(
            src2sinks.size(), vector<vector<vector<bool>>>(
                W, vector<vector<bool>>(
                    numSB - 1, vector<bool>(
                        numSB, 0
                    )
                )
            )
        );
        singleNetUseMatrix['h'] = vector<vector<vector<vector<bool>>>>(
            src2sinks.size(), vector<vector<vector<bool>>>(
                W, vector<vector<bool>>(
                    numSB, vector<bool>(
                        numSB - 1, 0
                    )
                )
            )
        );
    }

    // Path finder
    vector<PathNode*> genNeighbors(PathNode* cur) {

        vector<PathNode*> neighbors;
        if (cur->isPin) {
            // Get neighboring segments
            PinLoc pin = cur->pin;
            if (pin.p != 4) {
                return {};
            }
            int r = N - 1 - pin.y;
            int c = pin.x;
            for (int l = 0; l < W; ++l) {
                Segment   startSeg = layout[l][r][c][SOUTH];
                PathNode* startSegNode = new PathNode(false, PinLoc(), true, startSeg, costMatrix['v'][l][startSeg.r][startSeg.c]);
                neighbors.push_back(startSegNode);
            }
            return neighbors;

        }
        assert(cur->isSegment);


        Segment seg = cur->segment;
        ll totalCost = cur->totalCost;
        int r = seg.r, c = seg.c, l = seg.l;

        // Get the two switch blocks at the ends of the segment (r1,c1); (r2,c2)
        int r1 = r, c1 = c;
        int r2, c2;
        switch (seg.orient) {
        case 'v':
            r2 = r + 1;
            c2 = c;
            break;
        case 'h':
            r2 = r;
            c2 = c + 1;
            break;
        default:
            assert(0);
        }

        // Get neighboring pins and segments
        if (seg.orient == 'v') {
            Segment   upEast = layout[l][r1][c1][EAST];
            Segment   upNorth = layout[l][r1][c1][NORTH];
            Segment   upWest = layout[l][r1][c1][WEST];
            Segment   downEast = layout[l][r2][c2][EAST];
            Segment   downSouth = layout[l][r2][c2][SOUTH];
            Segment   downWest = layout[l][r2][c2][WEST];
            if (upEast.orient != 'u') {
                PathNode* upEastNode = new PathNode(false, PinLoc(), true, upEast, totalCost + costMatrix['h'][l][upEast.r][upEast.c]);
                neighbors.push_back(upEastNode);
            }
            if (upNorth.orient != 'u') {
                PathNode* upNorthNode = new PathNode(false, PinLoc(), true, upNorth, totalCost + costMatrix['v'][l][upNorth.r][upNorth.c]);
                neighbors.push_back(upNorthNode);
            }
            if (upWest.orient != 'u') {
                PathNode* upWestNode = new PathNode(false, PinLoc(), true, upWest, totalCost + costMatrix['h'][l][upWest.r][upWest.c]);
                neighbors.push_back(upWestNode);
            }
            if (downEast.orient != 'u') {
                PathNode* downEastNode = new PathNode(false, PinLoc(), true, downEast, totalCost + costMatrix['h'][l][downEast.r][downEast.c]);
                neighbors.push_back(downEastNode);
            }
            if (downSouth.orient != 'u') {
                PathNode* downSouthNode = new PathNode(false, PinLoc(), true, downSouth, totalCost + costMatrix['v'][l][downSouth.r][downSouth.c]);
                neighbors.push_back(downSouthNode);
            }
            if (downWest.orient != 'u') {
                PathNode* downWestNode = new PathNode(false, PinLoc(), true, downWest, totalCost + costMatrix['h'][l][downWest.r][downWest.c]);
                neighbors.push_back(downWestNode);
            }
            if (c > 0) {
                PinLoc    eastPinA = PinLoc('a', c - 1, N - 1 - r, 2);
                PathNode* eastPinANode = new PathNode(true, eastPinA, false, Segment(), totalCost);
                neighbors.push_back(eastPinANode);
                if (isDense) {
                    PinLoc    eastPinB = PinLoc('b', c - 1, N - 1 - r, 2);
                    PathNode* eastPinBNode = new PathNode(true, eastPinB, false, Segment(), totalCost);
                    neighbors.push_back(eastPinBNode);
                }
            }
            return neighbors;
        }
        if (seg.orient == 'h') {
            Segment   leftNorth = layout[l][r1][c1][NORTH];
            Segment   leftWest = layout[l][r1][c1][WEST];
            Segment   leftSouth = layout[l][r1][c1][SOUTH];
            Segment   rightSouth = layout[l][r2][c2][SOUTH];
            Segment   rightEast = layout[l][r2][c2][EAST];
            Segment   rightNorth = layout[l][r2][c2][NORTH];
            if (leftNorth.orient != 'u') {
                PathNode* leftNorthNode = new PathNode(false, PinLoc(), true, leftNorth, totalCost + costMatrix['v'][l][leftNorth.r][leftNorth.c]);
                neighbors.push_back(leftNorthNode);
            }
            if (leftWest.orient != 'u') {
                PathNode* leftWestNode = new PathNode(false, PinLoc(), true, leftWest, totalCost + costMatrix['h'][l][leftWest.r][leftWest.c]);
                neighbors.push_back(leftWestNode);
            }
            if (leftSouth.orient != 'u') {
                PathNode* leftSouthNode = new PathNode(false, PinLoc(), true, leftSouth, totalCost + costMatrix['v'][l][leftSouth.r][leftSouth.c]);
                neighbors.push_back(leftSouthNode);
            }
            if (rightSouth.orient != 'u') {
                PathNode* rightSouthNode = new PathNode(false, PinLoc(), true, rightSouth, totalCost + costMatrix['v'][l][rightSouth.r][rightSouth.c]);
                neighbors.push_back(rightSouthNode);
            }
            if (rightEast.orient != 'u') {
                PathNode* rightEastNode = new PathNode(false, PinLoc(), true, rightEast, totalCost + costMatrix['h'][l][rightEast.r][rightEast.c]);
                neighbors.push_back(rightEastNode);
            }
            if (rightNorth.orient != 'u') {
                PathNode* rightNorthNode = new PathNode(false, PinLoc(), true, rightNorth, totalCost + costMatrix['v'][l][rightNorth.r][rightNorth.c]);
                neighbors.push_back(rightNorthNode);
            }
            if (r > 0) {
                PinLoc    southPinA = PinLoc('a', c, N - 1 - (r - 1), 1);
                PathNode* southPinANode = new PathNode(true, southPinA, false, Segment(), totalCost);
                neighbors.push_back(southPinANode);
                if (isDense) {
                    PinLoc    southPinB = PinLoc('b', c, N - 1 - (r - 1), 1);
                    PathNode* southPinBNode = new PathNode(true, southPinB, false, Segment(), totalCost);
                    neighbors.push_back(southPinBNode);
                }
            }
            if (r < N) {
                PinLoc    northPinA = PinLoc('a', c, N - 1 - r, 3);
                PathNode* northPinANode = new PathNode(true, northPinA, false, Segment(), totalCost);
                neighbors.push_back(northPinANode);
                if (isDense) {
                    PinLoc    northPinB = PinLoc('b', c, N - 1 - r, 3);
                    PathNode* northPinBNode = new PathNode(true, northPinB, false, Segment(), totalCost);
                    neighbors.push_back(northPinBNode);
                }
            }
            return neighbors;
        }

        assert(0);
        return {};
    }
    void pathFinder(int net, PathNode* outputNode, const vector<PinLoc>& inputs, map<char, vector<vector<vector<bool>>>>& lut) {

        // Declare a min heap
        auto cmp = [&](const PathNode* n1, const PathNode* n2) {
            if (n1->totalCost == n2->totalCost) {
                if (n1->isSegment && n2->isSegment) {
                    return n1->segment.l > n2->segment.l;
                }
            }
            return (n1->totalCost > n2->totalCost);
            };
        priority_queue<PathNode*, vector<PathNode*>, decltype(cmp)> pq(cmp);

        // Keep track of the minimum cost to each segment or pin for the current route
        map<char, vector<vector<vector<ll>>>> singleRouteTotalCostMatrixOnSeg;
        singleRouteTotalCostMatrixOnSeg['v'] = vector<vector<vector<ll>>>(
            W, vector<vector<ll>>(
                N, vector<ll>(
                    N + 1, DBL_MAX
                )
            )
        );
        singleRouteTotalCostMatrixOnSeg['h'] = vector<vector<vector<ll>>>(
            W, vector<vector<ll>>(
                N + 1, vector<ll>(
                    N, DBL_MAX
                )
            )
        );
        map<char, vector<vector<vector<ll>>>> singleRouteTotalCostMatrixOnPin;
        singleRouteTotalCostMatrixOnPin['a'] = vector<vector<vector<ll>>>(
            N, vector<vector<ll>>(
                N, vector<ll>(
                    4, DBL_MAX
                )
            )
        );
        if (isDense) {
            singleRouteTotalCostMatrixOnPin['b'] = vector<vector<vector<ll>>>(
                N, vector<vector<ll>>(
                    N, vector<ll>(
                        4, DBL_MAX
                    )
                )
            );
        }
        // std::cout << "Single route initializations done" << endl;

        // Dijkstra
        vector<PathNode*> inputNodes;
        pq.push(outputNode);
        // std::cout << "Dijkstra start" << endl;
        while (!pq.empty()) {

            PathNode* cur = pq.top(); pq.pop();

            // Check if we have reached an end
            if (cur->isPin) {
                PinLoc pin = cur->pin;
                if (lut[pin.b][pin.x][pin.y][pin.p - 1]) {
                    inputNodes.push_back(cur);
                    if (inputNodes.size() == inputs.size()) {
                        break;
                    }
                }
            }

            // Generate neighbors
            // std::cout << "gen neighbors start" << endl;
            vector<PathNode*> neis = genNeighbors(cur);
            // std::cout << "gen neighbors end" << endl;
            for (PathNode* nei : neis) {
                if (nei->isSegment) {
                    Segment neiSeg = nei->segment;
                    ll neiTotalCost = nei->totalCost;
                    assert(neiSeg.orient != 'u');
                    // std::cout << "check single net usage start" << endl;
                    // std::cout << net << " " << src2sinks.size() << endl;
                    if (singleNetUseMatrix[neiSeg.orient][net][neiSeg.l][neiSeg.r][neiSeg.c] == 1) {
                        // std::cout << "skip single net usage" << endl;
                        continue; // The segment is already in the current net. Skip it
                    }
                    // std::cout << "check single net usage finish" << endl;
                    if (singleRouteTotalCostMatrixOnSeg[neiSeg.orient][neiSeg.l][neiSeg.r][neiSeg.c] > neiTotalCost) {
                        // Push the neighbor node into the heap only if the new total cost is smaller
                        singleRouteTotalCostMatrixOnSeg[neiSeg.orient][neiSeg.l][neiSeg.r][neiSeg.c] = neiTotalCost;
                        nei->parent = cur;
                        pq.push(nei);
                    }
                    // std::cout << "check single route cost finish" << endl;
                }
                else {
                    assert(nei->isPin);
                    PinLoc neiPin = nei->pin;
                    ll neiTotalCost = nei->totalCost;
                    if (lut[neiPin.b][neiPin.x][neiPin.y][neiPin.p - 1] == 0) continue; // The pin is not a target. Skip it.
                    if (singleRouteTotalCostMatrixOnPin[neiPin.b][neiPin.x][neiPin.y][neiPin.p - 1] > neiTotalCost) {
                        // Push the neighbor node into the heap only if the new total cost is smaller
                        singleRouteTotalCostMatrixOnPin[neiPin.b][neiPin.x][neiPin.y][neiPin.p - 1] = neiTotalCost;
                        nei->parent = cur;
                        pq.push(nei);
                    }
                }
            }
        }
        assert(inputNodes.size() == inputs.size());
        // std::cout << "Dijkstra end" << endl;

        // Back trace the path tree
        // std::cout << "Back trace start" << endl;
        for (PathNode* inputNode : inputNodes) {
            PathNode* cur = inputNode;
            while ((!cur->isLocated) && (cur != outputNode)) {
                assert(cur != NULL);
                // if (cur -> isPin) {
                //    // std::cout << cur->isLocated << " " << cur->pin.toStr() << endl;
                // }
                // if (cur -> isSegment)
                //    // std::cout << cur->isLocated << " " << cur->segment.orient << " " << cur->segment.l << " " << cur->segment.r << " " << cur->segment.c << endl; 

                PathNode* par = cur->parent;
                par->children.push_back(cur);
                cur->isLocated = 1;
                if (cur->isSegment) {
                    // std::cout << "Update cost matrix start" << endl;
                    // std::cout << cur->isLocated << " " << cur->segment.orient << " " << cur->segment.l << " " << cur->segment.r << " " << cur->segment.c << endl; 
                    // Update cost based on the new overuse condition
                    Segment seg = cur->segment;
                    assert(singleNetUseMatrix[seg.orient][net][seg.l][seg.r][seg.c] == 0);
                    singleNetUseMatrix[seg.orient][net][seg.l][seg.r][seg.c] = 1;
                    useMatrix[seg.orient][seg.l][seg.r][seg.c]++;
                    costMatrix[seg.orient][seg.l][seg.r][seg.c] = historyMatrix[seg.orient][seg.l][seg.r][seg.c] * (1 + overuse(seg)) * pFac;
                    noOverUse &= (overuse(seg) == 0);
                    // std::cout << "Update cost matrix end" << endl;
                }
                cur = par;
            }
            if (!cur->isLocated) {
                assert(cur == outputNode);
                cur->isLocated = 1;
            }
        }
        // std::cout << "Back trace end" << endl;
    }

    // Route all the paths
    void genRoutes() {

        // Route all nets
        cout << "##########################" << endl;
        cout << "###   Route all nets   ###" << endl;
        cout << "##########################" << endl;
        int net = 0;
        for (auto& p : src2sinks) {

            // Get the src and sinks ready
            PinLoc         src = str2Loc(p.first);
            PathNode* root = new PathNode(true, src, false, Segment(), (ll)0);
            vector<PinLoc> sinks = p.second;

            // Generate lookup table for target input pins
            map<char, vector<vector<vector<bool>>>> lut;
            lut['a'] = vector<vector<vector<bool>>>(N, vector<vector<bool>>(N, vector<bool>(4, 0)));
            if (isDense) {
                lut['b'] = vector<vector<vector<bool>>>(N, vector<vector<bool>>(N, vector<bool>(4, 0)));
            }
            for (const PinLoc& input : sinks) {
                lut[input.b][input.x][input.y][input.p - 1] = 1;
            }

            // Route a single path
            // std::cout << "start path finder" << endl;
            pathFinder(net, root, sinks, lut);
            assert(root->isLocated);
            routes.push_back(root);
            cout << "Path: " << p.first << " -> " << p.second.size() << endl;
            net++;

        }

        // Debug: print segments used more than once
        cout << " --- Overused segments ---" << endl;
        for (PathNode* root : routes) {
            assert(root->isPin && (root->pin.p == 4));
            string output = root->pin.toStr();
            vector<string> inputs;
            stack<PathNode*> st;
            st.push(root);
            while (!st.empty()) {
                PathNode* cur = st.top(); st.pop();
                if (cur->isSegment) {
                    Segment seg = cur->segment;
                    if (useMatrix[seg.orient][seg.l][seg.r][seg.c] > 1) {
                        cout << seg.orient << " " << seg.l << " " << seg.r << " " << seg.c << endl;
                    }
                }
                for (PathNode* child : cur->children) {
                    st.push(child);
                }
            }
        }
        cout << endl;

        // Nair's approach to re-route all nets
        for (int iter = 1; !noOverUse && iter < maxIterationNum; ++iter) {

            cout << "##########################" << endl;
            cout << "## Re-routing iteration #" << iter << endl;
            cout << "##########################" << endl;

            // Update histories and costs
            for (PathNode* root : routes) {
                stack<PathNode*> st;
                st.push(root);
                while (!st.empty()) {
                    PathNode* cur = st.top(); st.pop();
                    if (cur->isSegment) {
                        Segment seg = cur->segment;
                        historyMatrix[seg.orient][seg.l][seg.r][seg.c] += overuse(seg) * hFac;
                        costMatrix[seg.orient][seg.l][seg.r][seg.c] = historyMatrix[seg.orient][seg.l][seg.r][seg.c] * (1 + overuse(seg)) * pFac;
                    }
                    for (PathNode* child : cur->children) {
                        st.push(child);
                    }
                }
            }

            // Rip out and re-route
            noOverUse = true;
            int maxNumBackIterations = -1;
            for (int j = numOverUsedSegsHistory.size() - 1; j > 0; --j) {
                if (numOverUsedSegsHistory[j] != numOverUsedSegsHistory[j - 1]) {
                    // Don't want to get stuck
                    break;
                }
                maxNumBackIterations++;
            }
            cout << "Max number of ancestors going back: " << maxNumBackIterations << endl;
            for (int net = 0; net < routes.size(); ++net) {

                PathNode* root = routes[net];
                stack<pair<PathNode*, int>> st;
                st.push(make_pair(root, 0));
                while (!st.empty()) {

                    PathNode* cur = st.top().first;
                    int level = st.top().second;
                    st.pop();
                    bool isReRouteRequired = false;

                    // If there is at least one child that is overused, re-route all sub nets
                    for (PathNode* child : cur->children) {
                        if (child->isSegment && (overuse(child->segment) > 0)) {
                            isReRouteRequired = true;
                            break;
                        }
                    }
                    if (isReRouteRequired) {

                        // Find where to start re-routing
                        PathNode* start = cur;
                        for (int p = 0; p < 2 * maxNumBackIterations && start != root; ++p) {
                            start = start->parent;
                            level--;
                        }
                        assert(level >= 0);

                        // Rip out all the sub-trees of the start node
                        vector<PinLoc> leafPins;
                        stack<PathNode*> st2;
                        for (PathNode* child : start->children) {
                            st2.push(child);
                        }
                        while (!st2.empty()) {
                            PathNode* cur2 = st2.top(); st2.pop();
                            if (cur2->isSegment) {
                                Segment seg = cur2->segment;
                                assert(singleNetUseMatrix[seg.orient][net][seg.l][seg.r][seg.c] == 1);
                                singleNetUseMatrix[seg.orient][net][seg.l][seg.r][seg.c] = 0;
                                useMatrix[seg.orient][seg.l][seg.r][seg.c]--;
                                costMatrix[seg.orient][seg.l][seg.r][seg.c] = historyMatrix[seg.orient][seg.l][seg.r][seg.c] * (1 + overuse(seg)) * pFac;
                            }
                            else {
                                assert(cur2->isPin && (cur2->pin.p != 4));
                                leafPins.push_back(cur2->pin);
                            }
                            for (PathNode* child2 : cur2->children) {
                                st2.push(child2);
                            }
                        }

                        // Delete the old children and its successors to not run out of memory
                        for (PathNode* child : start->children) {
                            deleteTree(child);
                        }
                        start->children = {};

                        // Generate lookup table for reduced input pins
                        map<char, vector<vector<vector<bool>>>> lut;
                        lut['a'] = vector<vector<vector<bool>>>(N, vector<vector<bool>>(N, vector<bool>(4, 0)));
                        if (isDense) {
                            lut['b'] = vector<vector<vector<bool>>>(N, vector<vector<bool>>(N, vector<bool>(4, 0)));
                        }
                        for (const PinLoc& input : leafPins) {
                            lut[input.b][input.x][input.y][input.p - 1] = 1;
                        }

                        // Span a new sub net
                        pathFinder(net, start, leafPins, lut);

                        // Pop the stack until the node at the same or lower level appears
                        while (!st.empty()) {
                            int nodeLevel = st.top().second;
                            if (nodeLevel <= level) {
                                break;
                            }
                            st.pop();
                        }

                    }
                    else {

                        for (PathNode* child : cur->children) {
                            st.push(make_pair(child, level + 1));
                        }

                    }
                }
            }

            // Update pFac
            pFac *= 3.5;


            // Debug: print segments used more than once
            cout << " --- Overused segments ---" << endl;
            int numOverUsedSegs = 0;
            for (PathNode* root : routes) {
                assert(root->isPin && (root->pin.p == 4));
                string output = root->pin.toStr();
                vector<string> inputs;
                stack<PathNode*> st;
                st.push(root);
                while (!st.empty()) {
                    PathNode* cur = st.top(); st.pop();
                    if (cur->isSegment) {
                        Segment seg = cur->segment;
                        if (useMatrix[seg.orient][seg.l][seg.r][seg.c] > 1) {
                            cout << seg.orient << " " << seg.l << " " << seg.r << " " << seg.c << endl;
                            numOverUsedSegs++;
                        }
                    }
                    for (PathNode* child : cur->children) {
                        st.push(child);
                    }
                }
            }
            numOverUsedSegsHistory.push_back(numOverUsedSegs);
            cout << endl;
        }

        if (!noOverUse) {
            cout << "Routing failed :(" << endl;
        }
        else {
            cout << "Routing Success :)" << endl;
        }

        cout << "######################################" << endl;
        cout << "### Routing Result Verification... ###" << endl;
        cout << "######################################" << endl;
        noOverUse = true;
        useMatrix['v'] = vector<vector<vector<int>>>(
            W, vector<vector<int>>(
                N, vector<int>(
                    N + 1, 0
                )
            )
        );
        useMatrix['h'] = vector<vector<vector<int>>>(
            W, vector<vector<int>>(
                N + 1, vector<int>(
                    N, 0
                )
            )
        );
        net = 0;
        for (PathNode* root : routes) {
            assert(root->isPin && (root->pin.p == 4));
            string output = root->pin.toStr();
            vector<string> inputs;
            stack<PathNode*> st;
            st.push(root);
            while (!st.empty()) {
                PathNode* cur = st.top(); st.pop();
                if (cur->isSegment) {
                    Segment seg = cur->segment;
                    useMatrix[seg.orient][seg.l][seg.r][seg.c]++;
                }
                else {
                    assert(cur->isPin);
                    if (cur->pin.p != 4) {
                        inputs.push_back(cur->pin.toStr());
                    }
                    else {
                        assert(output == cur->pin.toStr());
                    }
                }
                for (PathNode* child : cur->children) {
                    st.push(child);
                }
            }
            for (char orient : {'v', 'h'}) {
                for (int l = 0; l < W; ++l) {
                    for (int r = 0; r < (orient == 'h' ? N + 1 : N); ++r) {
                        for (int c = 0; c < (orient == 'v' ? N + 1 : N); ++c) {
                            noOverUse &= (useMatrix[orient][l][r][c] <= 1);
                        }
                    }
                }
            }
            vector<PinLoc> inputPins = src2sinks[output];
            vector<string> expectedInputs;
            for (PinLoc inputPin : inputPins) {
                expectedInputs.push_back(inputPin.toStr());
            }
            sort(inputs.begin(), inputs.end());
            sort(expectedInputs.begin(), expectedInputs.end());
            assert(inputs.size() == expectedInputs.size());
            cout << "Source " + output + " -> Loads: ";
            for (int i = 0; i < inputs.size(); ++i) {
                assert(inputs[i] == expectedInputs[i]);
                cout << inputs[i] << " ";
            }
            cout << endl;

            net++;
        }
        if (noOverUse) {
            cout << "Routing results verified: Success !!!" << endl;
        }
        else {
            cout << "Routing results verified: Fail ..." << endl;
        }
    }

    // Delete all the routes to save memory
    void deleteRoutes() {
        for (PathNode* root : routes) {
            deleteTree(root);
        }
    }
};

#endif