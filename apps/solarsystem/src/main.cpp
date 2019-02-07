// main.cpp : Defines the entry point for the console application.
//
// Local Includes
#include "solarsystem.h"

// Nap includes
#include <apprunner.h>
#include <nap/logger.h>
#include <guiappeventhandler.h>

// Main loop
int main(int argc, char *argv[])
{
    // Create core
    nap::Core core;

    // Create app runner
    nap::AppRunner<nap::CoreApp, nap::GUIAppEventHandler> appRunner(core);

    // Decide which file to load
    if (argc >= 2) {
        // Command line provided
        appRunner.getApp().setFilename(argv[1]);
    } else {
        // Default
        appRunner.getApp().setFilename("default.json");
    }


    // Start
    nap::utility::ErrorState error;
    if (!appRunner.start(error))
    {
        nap::Logger::fatal("error: %s", error.toString().c_str());
        return -1;
    }

    // Return if the app ran successfully
    return appRunner.exitCode();
}

