#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <memory>

// Include modular CLI components
#include "../include/cli_command_processor.h"

int main(int argc, char** argv) {
    try {
        moneybot::CLICommandProcessor processor;
        return processor.processCommand(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
