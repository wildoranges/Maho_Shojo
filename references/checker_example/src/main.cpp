#include <iostream>
#include "C1Driver.h"
#include "SyntaxTreePrinter.h"
#include "SyntaxTreeChecker.h"

void print_help(std::string exe_name) {
  std::cout << "Usage: " << exe_name
            << " [ -h | --help ] [ -p | --trace_parsing ] [ -s | --trace_scanning ] [ -e | --emit_syntax ] "
            << "<input-file>"
            << std::endl;
}

int main(int argc, char *argv[])
{
    C1Driver driver;
    SyntaxTreePrinter printer;
    ErrorReporter reporter(std::cerr);
    SyntaxTreeChecker checker(reporter);

    bool print = false;
    std::string filename;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
            print_help(argv[0]);
            return 0;
        }
        else if (argv[i] == std::string("-p") || argv[i] == std::string("--trace_parsing"))
            driver.trace_parsing = true;
        else if (argv[i] == std::string("-s") || argv[i] == std::string("--trace_scanning"))
            driver.trace_scanning = true;
        else if (argv[i] == std::string("-e") || argv[i] == std::string("--emit_syntax"))
            print = true;
        else {
            filename = argv[i];
        }
    }

    auto root = driver.parse(filename);
    if (print)
        root->accept(printer);
    root->accept(checker);
    if(checker.is_err())
    {
        std::cout<<"The file has semantic errors\n";
        exit(1);
    }
    std::cout<<"pass\n";
    return 0;
}
