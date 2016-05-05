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

    tbricks_test::callback_t a_callback = tbricks_test::print_to_stdout;
    tbricks_test::err_callback_t an_err_callback =
            tbricks_test::print_to_stderr;
    tbricks_test::feed_handler a_feed_handler{symbol,
        std::move(a_callback), std::move(an_err_callback)};

    std::string line;
    while (std::getline(infile, line)) {
        a_feed_handler.process_command(line);
    }
}
