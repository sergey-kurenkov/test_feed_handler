#include <iostream>
#include <fstream>
#include <string>

#include "feed_handler.h"


int main(int argc, char* argv[]) {
    if (argc == 1 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <file> [<symbol>]" << std::endl;
        return 1;
    }

    std::string symbol, file;
    if (argc == 3) {
        symbol = argv[2];
    }
    file = argv[1];

    std::ifstream infile(file);
    if (infile.fail()) {
        std::cerr << "File " << file << " does not exists"  << std::endl;
        return 1;
    }

    test_ns::callback_t a_callback = test_ns::print_to_stdout;
    test_ns::err_callback_t an_err_callback =
            test_ns::print_to_stderr;
    test_ns::feed_handler a_feed_handler{symbol,
        std::move(a_callback), std::move(an_err_callback)};

    std::string line;
    while (std::getline(infile, line)) {
        a_feed_handler.process_command(line);
    }
}
