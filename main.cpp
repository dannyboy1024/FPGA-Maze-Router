
#include "util.h"
#include "test.h"
#include "gui.h"
#include "solver.h"

using namespace std;

int main(int argc, char* argv[]) {

    // Get all tests
    bool isDebuggingMode = false;
    vector<string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    vector<Test> tests;
    for (string arg : args) {
        if (arg == "detail") {
            isDebuggingMode = true;
        }
        else {
            tests.push_back(genTest(arg));
        }
    }

    // Solve all tests
    for (Test test : tests) {

        // Update the gui
        gui.updateConfig(test);
        gui_render_board();

        // Initialize the solver
        Solver solver = Solver(test);

        // Generate all the routes
        solver.genRoutes();
        gui.setRoutes(solver.routes);
        solver.deleteRoutes();

        // Render all the routes and wait for user operations
        gui_render_all(isDebuggingMode);
    }

    close_graphics();
    printf("Graphics closed down.\n");

    return 0;
}