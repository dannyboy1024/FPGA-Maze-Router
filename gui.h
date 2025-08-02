#ifndef GUI_H
#define GUI_H

#include "util.h"

using namespace std;

class SB {
public:
    float x1, y1; // up left
    float x2, y2; // bottom right
    SB() {
        x1 = 0;
        x2 = 0;
        y1 = 0;
        y2 = 0;
    }
    SB(float _x1, float _y1, float blockLen) {
        x1 = _x1;
        y1 = _y1;
        x2 = x1 + blockLen;
        y2 = y1 + blockLen;
    }
    void draw(int r, int c) {
        setcolor(DARKGREEN);
        drawrect(x1, y1, x2, y2);
        float cx = (x1 + x2) / 2.f, cy = (y1 + y2) / 2.f;
        string pos = to_string(r) + "," + to_string(c);
        // setfontsize (111.111f / 5.f);
        setfontsize(18);
        drawtext(cx, cy, pos.c_str(), x2 - x1);
    }
};

class LB {
public:
    float x1, y1; // up left
    float x2, y2; // bottom right
    vector<float> px1, py1, px2, py2; // pin locations
    LB() {
        x1 = 0;
        x2 = 0;
        y1 = 0;
        y2 = 0;
    }
    LB(float _x1, float _y1, float blockLen, vector<float> pinLens) {
        // Left top
        x1 = _x1;
        y1 = _y1;
        // Right bottom
        x2 = x1 + blockLen;
        y2 = y1 + blockLen;
        // Pins
        px1.resize(4);
        py1.resize(4);
        px2.resize(4);
        py2.resize(4);
        // Pin 1
        px1[SOUTH] = (x2 - x1) / 4 * 3 + x1;
        py1[SOUTH] = y2;
        px2[SOUTH] = px1[SOUTH];
        py2[SOUTH] = py1[SOUTH] + pinLens[SOUTH];
        // Pin 2
        px1[EAST] = x2;
        py1[EAST] = (y2 - y1) / 2 + y1;
        px2[EAST] = px1[EAST] + pinLens[EAST];
        py2[EAST] = py1[EAST];
        // Pin 3
        px1[NORTH] = (x2 - x1) / 2 + x1;
        py1[NORTH] = y1;
        px2[NORTH] = px1[NORTH];
        py2[NORTH] = py1[NORTH] - pinLens[NORTH];
        // Pin 4
        px1[WEST] = x1;
        py1[WEST] = (y2 - y1) / 4 * 3 + y1;
        px2[WEST] = px1[WEST] - pinLens[WEST];
        py2[WEST] = py1[WEST];
    }
    void draw(string c, int xLoc, int yLoc) {
        setcolor(BLUE);
        setlinestyle(SOLID);
        setlinewidth(2);
        drawrect(x1, y1, x2, y2);
        for (int dir = 0; dir < 4; ++dir) {
            drawline(px1[dir], py1[dir], px2[dir], py2[dir]);
        }
        float cx = (x1 + x2) / 2, cy = (y1 + y2) / 2;
        string pos = to_string(xLoc) + "," + to_string(yLoc) + "(" + c + ")";
        setcolor(BLUE);
        setfontsize(10);
        drawtext(cx, cy, pos.c_str(), x2 - x1);
    }
};

class Track {
public:
    float x1, y1; // left/up
    float x2, y2; // right/down
    Track() {
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = 0;
    }
    Track(float _x1, float _y1, char orient, float trackLen) {
        x1 = _x1;
        y1 = _y1;
        x2 = x1 + (orient == 'h' ? trackLen : 0);
        y2 = y1 + (orient == 'v' ? trackLen : 0);
    }
    void draw() {
        setcolor(BLACK);
        setlinestyle(SOLID);
        setlinewidth(1);
        drawline(x1, y1, x2, y2);
    }
};

class Gui {
private:
    vector<PathNode*> tmpDfsPath;
    void dfs(PathNode* cur) {

        // Check if an input pin is reached so we are at the end of a path
        if (cur->isPin && (cur->pin.p != 4)) {

            // Get number of switch blocks along the path
            int numSegments = tmpDfsPath.size() - 1;
            int numSB = numSegments - 1;
            vector<vector<float>> path(1 + 2 * numSB + 1);

            // Switch block connections
            for (int i = 1; i < numSegments; ++i) {
                PathNode* src = tmpDfsPath[i], * sink = tmpDfsPath[i + 1]; assert(src->isSegment && sink->isSegment);
                Segment srcSeg = src->segment, sinkSeg = sink->segment; assert(srcSeg.l == sinkSeg.l);
                if (srcSeg.orient == 'v') {
                    if (sinkSeg.orient == 'v') {
                        assert(sinkSeg.c == srcSeg.c);
                        Track srcTrack = tracks['v'][srcSeg.r][srcSeg.c][srcSeg.l];
                        Track sinkTrack = tracks['v'][sinkSeg.r][sinkSeg.c][sinkSeg.l];
                        if (sinkSeg.r == srcSeg.r - 1) {
                            path[2 * i - 1] = { srcTrack.x1, srcTrack.y1 };
                            path[2 * i] = { sinkTrack.x2, sinkTrack.y2 };
                        }
                        else {
                            assert(sinkSeg.r == srcSeg.r + 1);
                            path[2 * i - 1] = { srcTrack.x2, srcTrack.y2 };
                            path[2 * i] = { sinkTrack.x1, sinkTrack.y1 };
                        }
                    }
                    else {
                        assert(sinkSeg.orient == 'h');
                        Track srcTrack = tracks['v'][srcSeg.r][srcSeg.c][srcSeg.l];
                        Track sinkTrack = tracks['h'][sinkSeg.r][sinkSeg.c][sinkSeg.l];
                        if (sinkSeg.r == srcSeg.r) {
                            if (sinkSeg.c == srcSeg.c) {
                                path[2 * i - 1] = { srcTrack.x1, srcTrack.y1 };
                                path[2 * i] = { sinkTrack.x1, sinkTrack.y1 };
                            }
                            else {
                                assert(sinkSeg.c == srcSeg.c - 1);
                                path[2 * i - 1] = { srcTrack.x1, srcTrack.y1 };
                                path[2 * i] = { sinkTrack.x2, sinkTrack.y2 };
                            }
                        }
                        else {
                            assert(sinkSeg.r == srcSeg.r + 1);
                            if (sinkSeg.c == srcSeg.c) {
                                path[2 * i - 1] = { srcTrack.x2, srcTrack.y2 };
                                path[2 * i] = { sinkTrack.x1, sinkTrack.y1 };
                            }
                            else {
                                assert(sinkSeg.c == srcSeg.c - 1);
                                path[2 * i - 1] = { srcTrack.x2, srcTrack.y2 };
                                path[2 * i] = { sinkTrack.x2, sinkTrack.y2 };
                            }
                        }
                    }
                }
                else {
                    assert(srcSeg.orient == 'h');
                    if (sinkSeg.orient == 'h') {
                        assert(sinkSeg.r == srcSeg.r);
                        Track srcTrack = tracks['h'][srcSeg.r][srcSeg.c][srcSeg.l];
                        Track sinkTrack = tracks['h'][sinkSeg.r][sinkSeg.c][sinkSeg.l];
                        if (sinkSeg.c == srcSeg.c - 1) {
                            path[2 * i - 1] = { srcTrack.x1, srcTrack.y1 };
                            path[2 * i] = { sinkTrack.x2, sinkTrack.y2 };
                        }
                        else {
                            assert(sinkSeg.c == srcSeg.c + 1);
                            path[2 * i - 1] = { srcTrack.x2, srcTrack.y2 };
                            path[2 * i] = { sinkTrack.x1, sinkTrack.y1 };
                        }
                    }
                    else {
                        assert(sinkSeg.orient == 'v');
                        Track srcTrack = tracks['h'][srcSeg.r][srcSeg.c][srcSeg.l];
                        Track sinkTrack = tracks['v'][sinkSeg.r][sinkSeg.c][sinkSeg.l];
                        if (sinkSeg.r == srcSeg.r) {
                            if (sinkSeg.c == srcSeg.c) {
                                path[2 * i - 1] = { srcTrack.x1, srcTrack.y1 };
                                path[2 * i] = { sinkTrack.x1, sinkTrack.y1 };
                            }
                            else {
                                assert(sinkSeg.c == srcSeg.c + 1);
                                path[2 * i - 1] = { srcTrack.x2, srcTrack.y2 };
                                path[2 * i] = { sinkTrack.x1, sinkTrack.y1 };
                            }
                        }
                        else {
                            assert(sinkSeg.r == srcSeg.r - 1);
                            if (sinkSeg.c == srcSeg.c) {
                                path[2 * i - 1] = { srcTrack.x1, srcTrack.y1 };
                                path[2 * i] = { sinkTrack.x2, sinkTrack.y2 };
                            }
                            else {
                                assert(sinkSeg.c == srcSeg.c + 1);
                                path[2 * i - 1] = { srcTrack.x2, srcTrack.y2 };
                                path[2 * i] = { sinkTrack.x2, sinkTrack.y2 };
                            }
                        }
                    }
                }
            }

            // Begin
            PathNode* start = tmpDfsPath[0]; assert(start->isPin && (start->pin.p == 4));
            PinLoc output = start->pin;
            LB outputLB = logicBlocks[N - 1 - output.y][output.x][output.b];
            Segment firstSeg = tmpDfsPath[1]->segment;
            Track firstTrack = tracks[firstSeg.orient][firstSeg.r][firstSeg.c][firstSeg.l];
            path[0] = { firstTrack.x1, outputLB.py1[WEST] };

            // End
            PinLoc input = cur->pin;
            LB inputLB = logicBlocks[N - 1 - input.y][input.x][input.b];
            Segment lastSeg = tmpDfsPath.back()->segment;
            Track lastTrack = tracks[lastSeg.orient][lastSeg.r][lastSeg.c][lastSeg.l];
            if (input.p == 2) {
                assert(lastSeg.orient == 'v');
                path[1 + 2 * numSB] = { lastTrack.x1, inputLB.py1[EAST] };
            }
            else if (input.p == 1) {
                assert(lastSeg.orient == 'h');
                path[1 + 2 * numSB] = { inputLB.px1[SOUTH], lastTrack.y1 };
            }
            else {
                assert(input.p == 3);
                assert(lastSeg.orient == 'h');
                path[1 + 2 * numSB] = { inputLB.px1[NORTH], lastTrack.y1 };
            }

            paths.push_back(path);
            return;
        }

        // Depth-first traversal
        tmpDfsPath.push_back(cur);
        for (PathNode* child : cur->children) {
            dfs(child);
        }
        tmpDfsPath.pop_back();
    }
public:
    float TRACK_BLOCK_RATIO;
    float LONG_PIN_BLOCK_RATIO;
    float SHORT_PIN_BLOCK_RATIO;
    float LB_A_X_OFFSET_BLOCK_RATIO;
    float LB_A_Y_OFFSET_BLOCK_RATIO;
    float LB_B_OFFSET_BLOCK_RATIO;
    float FONT_WIDTH_BLOCK_W_RAITO;
    float FONT_HEIGHT_BLOCK_W_RAITO;
    float TRACK_BEGIN_BLOCK_RATIO;
    int   N, W;
    float X, Y;
    float gridLen;
    bool  isDense;
    float blockLen;
    float trackLen;
    vector<float> pinLensA;
    vector<float> pinLensB;
    vector<vector<SB>> switchBlocks;
    vector<vector<map<char, LB>>> logicBlocks;
    map<char, vector<vector<vector<Track>>>> tracks;
    vector<vector<vector<float>>> paths;

    // For Debugging mode
    int pi;

    Gui(float _X, float _Y, float _gridLen) {

        // Initialize parameters
        TRACK_BLOCK_RATIO = 3.0;
        LONG_PIN_BLOCK_RATIO = 2.6667;
        SHORT_PIN_BLOCK_RATIO = 2.0;
        LB_A_X_OFFSET_BLOCK_RATIO = 0.4;
        LB_A_Y_OFFSET_BLOCK_RATIO = 0.5;
        LB_B_OFFSET_BLOCK_RATIO = 0.125;
        FONT_WIDTH_BLOCK_W_RAITO = 0.25;
        FONT_HEIGHT_BLOCK_W_RAITO = 0.4;
        TRACK_BEGIN_BLOCK_RATIO = 0.125;
        X = _X;
        Y = _Y;
        gridLen = _gridLen;
        tmpDfsPath = {};
        pi = 0;

        // Initialize graphics settings
        printf("About to start Gui.\n");
        init_graphics("FPGA", WHITE);
        init_world(0., 0., 1000., 1000.);
        update_message("Gui Initialization is done");\

    }
    void updateConfig(Test test) {

        // Update the gui given new FPGA architecture
        isDense = test.isDense;
        N = test.N;
        W = test.W;
        switchBlocks = vector<vector<SB>>(N + 1, vector<SB>(N + 1));
        logicBlocks = vector<vector<map<char, LB>>>(N, vector<map<char, LB>>(N));
        tracks['h'] = vector<vector<vector<Track>>>(N + 1, vector<vector<Track>>(N, vector<Track>(W)));
        tracks['v'] = vector<vector<vector<Track>>>(N, vector<vector<Track>>(N + 1, vector<Track>(W)));
        paths = {};

        // Set length of tracks and pins.
        // Set Width of blocks
        blockLen = gridLen / (N + TRACK_BLOCK_RATIO * (N - 1)); // (N * blockLen + (N-1) * trackLen = gridLen)
        trackLen = TRACK_BLOCK_RATIO * blockLen;
        pinLensA = { LONG_PIN_BLOCK_RATIO * blockLen, LONG_PIN_BLOCK_RATIO * blockLen, SHORT_PIN_BLOCK_RATIO * blockLen, SHORT_PIN_BLOCK_RATIO * blockLen };
        pinLensB = { SHORT_PIN_BLOCK_RATIO * blockLen, SHORT_PIN_BLOCK_RATIO * blockLen, LONG_PIN_BLOCK_RATIO * blockLen, LONG_PIN_BLOCK_RATIO * blockLen };

        // Set positions for each switch block on the screen
        for (int r = 0; r < N + 1; ++r) {
            for (int c = 0; c < N + 1; ++c) {
                float x1 = X + c * (blockLen + trackLen);
                float y1 = Y + r * (blockLen + trackLen);
                switchBlocks[r][c] = SB(x1, y1, blockLen);
            }
        }

        // Set positions for each Logic block and their pins on the screen
        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                SB sb = switchBlocks[r][c];
                float x1 = sb.x2 + LB_A_X_OFFSET_BLOCK_RATIO * blockLen;
                float y1 = sb.y2 + LB_A_Y_OFFSET_BLOCK_RATIO * blockLen;
                logicBlocks[r][c]['a'] = LB(x1, y1, blockLen, pinLensA);
                if (isDense) {
                    x1 += blockLen + LB_B_OFFSET_BLOCK_RATIO * blockLen;
                    y1 += blockLen;
                    logicBlocks[r][c]['b'] = LB(x1, y1, blockLen, pinLensB);
                }
            }
        }

        // Set positions for each horizontal track on the screen
        float v = blockLen / (W + 1);
        for (int r = 0; r < N + 1; ++r) {
            for (int c = 0; c < N; ++c) {
                for (int l = 0; l < W; ++l) {
                    SB sb = switchBlocks[r][c];
                    float x1 = sb.x2;
                    float y1 = sb.y1 + v * (l + 1);
                    tracks['h'][r][c][l] = Track(x1, y1, 'h', trackLen);
                }
            }
        }

        // Set positions for each vertical track on the screen
        float h = blockLen / (W + 1);
        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N + 1; ++c) {
                for (int l = 0; l < W; ++l) {
                    SB sb = switchBlocks[r][c];
                    float x1 = sb.x1 + h * (l + 1);
                    float y1 = sb.y2;
                    tracks['v'][r][c][l] = Track(x1, y1, 'v', trackLen);
                }
            }
        }

    }
    void setRoutes(const vector<PathNode*>& routes) {
        paths = {};
        for (PathNode* root : routes) {
            dfs(root);
        }
    }
};
Gui gui = Gui(0., 0., 1000.); // Initialize gui as a global variable
void gui_button_press(float x, float y) {
}
void gui_render_board() {

    bool isDense = gui.isDense;
    int  N = gui.N;
    int  W = gui.W;

    // Clear the screen
    set_draw_mode(DRAW_NORMAL);
    clearscreen();

    // Draw all the logic blocks with labels and their pins 
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            gui.logicBlocks[r][c]['a'].draw("a", c, N - 1 - r);
            // pin labels
            float sx = gui.logicBlocks[r][c]['a'].px1[SOUTH], sy = gui.logicBlocks[r][c]['a'].py1[SOUTH];
            float ex = gui.logicBlocks[r][c]['a'].px1[EAST], ey = gui.logicBlocks[r][c]['a'].py1[EAST];
            float nx = gui.logicBlocks[r][c]['a'].px1[NORTH], ny = gui.logicBlocks[r][c]['a'].py1[NORTH];
            float wx = gui.logicBlocks[r][c]['a'].px1[WEST], wy = gui.logicBlocks[r][c]['a'].py1[WEST];
            float dist2Center = 0.5 * gui.FONT_HEIGHT_BLOCK_W_RAITO * (gui.blockLen / 2.0);
            float fontBoundingBox = gui.blockLen / 8.f;
            setcolor(BLUE);
            // setfontsize (111.111f / 8.f);
            setfontsize(14);
            drawtext(sx, sy - dist2Center, "1", fontBoundingBox);
            drawtext(ex - dist2Center, ey, "2", fontBoundingBox);
            drawtext(nx, ny + dist2Center, "3", fontBoundingBox);
            drawtext(wx + dist2Center, wy, "4", fontBoundingBox);
            if (isDense) {
                gui.logicBlocks[r][c]['b'].draw("b", c, N - 1 - r);
                // pin labels
                float sx = gui.logicBlocks[r][c]['b'].px1[SOUTH], sy = gui.logicBlocks[r][c]['b'].py1[SOUTH];
                float ex = gui.logicBlocks[r][c]['b'].px1[EAST], ey = gui.logicBlocks[r][c]['b'].py1[EAST];
                float nx = gui.logicBlocks[r][c]['b'].px1[NORTH], ny = gui.logicBlocks[r][c]['b'].py1[NORTH];
                float wx = gui.logicBlocks[r][c]['b'].px1[WEST], wy = gui.logicBlocks[r][c]['b'].py1[WEST];
                float dist2Center = 0.5 * gui.FONT_HEIGHT_BLOCK_W_RAITO * (gui.blockLen / 2.0);
                float fontBoundingBox = gui.blockLen / 8.f;
                setcolor(BLUE);
                // setfontsize (111.111f / 8.f);
                setfontsize(14);
                drawtext(sx, sy - dist2Center, "1", fontBoundingBox);
                drawtext(ex - dist2Center, ey, "2", fontBoundingBox);
                drawtext(nx, ny + dist2Center, "3", fontBoundingBox);
                drawtext(wx + dist2Center, wy, "4", fontBoundingBox);
            }
        }
    }

    // Draw all the switch blocks
    for (int r = 0; r < N + 1; ++r) {
        for (int c = 0; c < N + 1; ++c) {
            gui.switchBlocks[r][c].draw(r, c);
        }
    }

    // Draw all the tracks
    for (int r = 0; r < N + 1; ++r) {
        for (int c = 0; c < N; ++c) {
            for (int l = 0; l < W; ++l) {
                gui.tracks['h'][r][c][l].draw();
            }
        }
    }
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N + 1; ++c) {
            for (int l = 0; l < W; ++l) {
                gui.tracks['v'][r][c][l].draw();
            }
        }
    }

    // Draw all possible connections in each switch block
    // Label each track
    // Highlight each track's beginning
    for (int r = 0; r < N + 1; ++r) {
        for (int c = 0; c < N + 1; ++c) {
            for (int l = 0; l < W; ++l) {
                float sx = r == N ? -1 : gui.tracks['v'][r][c][l].x1;
                float sy = r == N ? -1 : gui.tracks['v'][r][c][l].y1;
                float ex = c == N ? -1 : gui.tracks['h'][r][c][l].x1;
                float ey = c == N ? -1 : gui.tracks['h'][r][c][l].y1;
                float nx = r == 0 ? -1 : gui.tracks['v'][r - 1][c][l].x2;
                float ny = r == 0 ? -1 : gui.tracks['v'][r - 1][c][l].y2;
                float wx = c == 0 ? -1 : gui.tracks['h'][r][c - 1][l].x2;
                float wy = c == 0 ? -1 : gui.tracks['h'][r][c - 1][l].y2;

                // connections
                setcolor(BLACK);
                setlinestyle(DASHED);
                setlinewidth(1);
                if (sx != -1 && sy != -1 && ex != -1 && ey != -1) drawline(sx, sy, ex, ey);
                if (sx != -1 && sy != -1 && nx != -1 && ny != -1) drawline(sx, sy, nx, ny);
                if (sx != -1 && sy != -1 && wx != -1 && wy != -1) drawline(sx, sy, wx, wy);
                if (ex != -1 && ey != -1 && nx != -1 && ny != -1) drawline(ex, ey, nx, ny);
                if (ex != -1 && ey != -1 && wx != -1 && wy != -1) drawline(ex, ey, wx, wy);
                if (nx != -1 && ny != -1 && wx != -1 && wy != -1) drawline(nx, ny, wx, wy);

                // labels
                float dist2Center = 0.5 * gui.FONT_HEIGHT_BLOCK_W_RAITO * (gui.blockLen / (gui.W + 1));
                float fontBoundingBox = 3.0 * gui.FONT_WIDTH_BLOCK_W_RAITO * (gui.blockLen / (gui.W + 1));
                setcolor(DARKGREEN);
                setfontsize(10);
                if (sy != -1) drawtext(sx, sy - dist2Center, to_string(l).c_str(), fontBoundingBox);
                if (ex != -1) drawtext(ex - dist2Center, ey, to_string(l).c_str(), fontBoundingBox);
                if (ny != -1) drawtext(nx, ny + dist2Center, to_string(l).c_str(), fontBoundingBox);
                if (wx != -1) drawtext(wx + dist2Center, wy, to_string(l).c_str(), fontBoundingBox);

                // tracks' beginnings
                setcolor(DARKGREEN);
                setlinestyle(SOLID);
                setlinewidth(2);
                float dist2end = gui.TRACK_BEGIN_BLOCK_RATIO * gui.blockLen;
                if (sy != -1) drawline(sx, sy, sx, sy + dist2end);
                if (ex != -1) drawline(ex, ey, ex + dist2end, ey);
                if (ny != -1) drawline(nx, ny, nx, ny - dist2end);
                if (wx != -1) drawline(wx, wy, wx - dist2end, wy);
            }
        }
    }
}
void gui_render_up_to_latest() {

    // Render the board
    gui_render_board();

    // Draw all routes up-to-latest
    setlinewidth(3);
    setlinestyle(SOLID);
    setcolor(RED);
    for (int i = 0; i < gui.pi; ++i) {
        vector<vector<float>>& path = gui.paths[i];
        for (int j = 0; j < path.size() - 1; ++j) {
            vector<float> start = path[j], end = path[j + 1];
            drawline(start[0], start[1], end[0], end[1]);
        }
    }
    if (gui.pi < gui.paths.size()) {
        setcolor(MAGENTA);
        vector<vector<float>>& path = gui.paths[gui.pi];
        for (int j = 0; j < path.size() - 1; ++j) {
            vector<float> start = path[j], end = path[j + 1];
            drawline(start[0], start[1], end[0], end[1]);
        }
    }
}
void gui_render_routes(bool isDebuggingMode) {
    bool isDense = gui.isDense;
    int  N = gui.N;
    int  W = gui.W;

    // Draw all the tracks
    for (int r = 0; r < N + 1; ++r) {
        for (int c = 0; c < N; ++c) {
            for (int l = 0; l < W; ++l) {
                gui.tracks['h'][r][c][l].draw();
            }
        }
    }
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N + 1; ++c) {
            for (int l = 0; l < W; ++l) {
                gui.tracks['v'][r][c][l].draw();
            }
        }
    }

    // Draw all possible connections in each switch block
    // Label each track
    // Highlight each track's beginning
    for (int r = 0; r < N + 1; ++r) {
        for (int c = 0; c < N + 1; ++c) {
            for (int l = 0; l < W; ++l) {
                float sx = r == N ? -1 : gui.tracks['v'][r][c][l].x1;
                float sy = r == N ? -1 : gui.tracks['v'][r][c][l].y1;
                float ex = c == N ? -1 : gui.tracks['h'][r][c][l].x1;
                float ey = c == N ? -1 : gui.tracks['h'][r][c][l].y1;
                float nx = r == 0 ? -1 : gui.tracks['v'][r - 1][c][l].x2;
                float ny = r == 0 ? -1 : gui.tracks['v'][r - 1][c][l].y2;
                float wx = c == 0 ? -1 : gui.tracks['h'][r][c - 1][l].x2;
                float wy = c == 0 ? -1 : gui.tracks['h'][r][c - 1][l].y2;

                // connections
                setcolor(BLACK);
                setlinestyle(DASHED);
                setlinewidth(1);
                if (sx != -1 && sy != -1 && ex != -1 && ey != -1) drawline(sx, sy, ex, ey);
                if (sx != -1 && sy != -1 && nx != -1 && ny != -1) drawline(sx, sy, nx, ny);
                if (sx != -1 && sy != -1 && wx != -1 && wy != -1) drawline(sx, sy, wx, wy);
                if (ex != -1 && ey != -1 && nx != -1 && ny != -1) drawline(ex, ey, nx, ny);
                if (ex != -1 && ey != -1 && wx != -1 && wy != -1) drawline(ex, ey, wx, wy);
                if (nx != -1 && ny != -1 && wx != -1 && wy != -1) drawline(nx, ny, wx, wy);

                // labels
                float dist2Center = 0.5 * gui.FONT_HEIGHT_BLOCK_W_RAITO * (gui.blockLen / (gui.W + 1));
                float fontBoundingBox = 3.0 * gui.FONT_WIDTH_BLOCK_W_RAITO * (gui.blockLen / (gui.W + 1));
                setcolor(DARKGREEN);
                setfontsize(10);
                if (sy != -1) drawtext(sx, sy - dist2Center, to_string(l).c_str(), fontBoundingBox);
                if (ex != -1) drawtext(ex - dist2Center, ey, to_string(l).c_str(), fontBoundingBox);
                if (ny != -1) drawtext(nx, ny + dist2Center, to_string(l).c_str(), fontBoundingBox);
                if (wx != -1) drawtext(wx + dist2Center, wy, to_string(l).c_str(), fontBoundingBox);

                // tracks' beginnings
                setcolor(DARKGREEN);
                setlinestyle(SOLID);
                setlinewidth(2);
                float dist2end = gui.TRACK_BEGIN_BLOCK_RATIO * gui.blockLen;
                if (sy != -1) drawline(sx, sy, sx, sy + dist2end);
                if (ex != -1) drawline(ex, ey, ex + dist2end, ey);
                if (ny != -1) drawline(nx, ny, nx, ny - dist2end);
                if (wx != -1) drawline(wx, wy, wx - dist2end, wy);
            }
        }
    }

    // Draw all routes
    setcolor(RED);
    setlinewidth(3);
    setlinestyle(SOLID);
    for (int i = 0; i < gui.paths.size(); ++i) {
        vector<vector<float>>& path = gui.paths[i]; gui.pi = i;
        if (isDebuggingMode) {
            setcolor(MAGENTA);
            for (int j = 0; j < path.size() - 1; ++j) {
                vector<float> start = path[j], end = path[j + 1];
                drawline(start[0], start[1], end[0], end[1]);
            }
            event_loop(gui_button_press, NULL, NULL, gui_render_up_to_latest);
        }
        setcolor(RED);
        for (int j = 0; j < path.size() - 1; ++j) {
            vector<float> start = path[j], end = path[j + 1];
            drawline(start[0], start[1], end[0], end[1]);
        }
    }
    gui.pi++; assert(gui.pi == gui.paths.size());
    event_loop(gui_button_press, NULL, NULL, gui_render_up_to_latest);
}
void gui_render_all(bool isDebuggingMode) {
    gui_render_board();
    gui_render_routes(isDebuggingMode);
}

# endif
