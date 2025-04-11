#include "core/Application.h"
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#endif

/**
 * @brief Main entry point for the application.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Exit code of the application.
 */
int main(int argc, char* argv[])
{
    try {
        // Create the application instance
        poe::Application app("PoEOverlay");
        
        // Initialize the application
        if (!app.Initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }
        
        // Run the application
        int exitCode = app.Run();
        
        // Return the exit code
        return exitCode;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
}

#ifdef _WIN32
/**
 * @brief Windows entry point for the application.
 * @param hInstance Handle to the current instance of the application.
 * @param hPrevInstance Handle to the previous instance of the application.
 * @param lpCmdLine Command line for the application.
 * @param nCmdShow Controls how the window is shown.
 * @return Exit code of the application.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif