#include <iostream>
#include "MHSJBuilder.hpp"
#include "MHSJDriver.h"
#include "SyntaxTreePrinter.h"
#include "SyntaxTreeChecker.h"

void print_help(std::string exe_name) {
  std::cout << "Usage: " << exe_name
            << " [ -h | --help ] [ -p | --trace_parsing ] [ -s | --trace_scanning ] [ -emit-mir ] [ -emit-ast ] [-nocheck] [-o <output-file>]"
            << "<input-file>"
            << std::endl;
}

int main(int argc, char *argv[])
{
    MHSJBuilder builder;
    MHSJDriver driver;
    SyntaxTreePrinter printer;
    ErrorReporter reporter(std::cerr);
    SyntaxTreeChecker checker(reporter);

    bool print_ast = false;
    bool print_IR = false;
    bool check = true;
    std::string out_file = "a.ll";
    std::string filename = "test.sysy";
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
            print_help(argv[0]);
            return 0;
        }
        else if (argv[i] == std::string("-p") || argv[i] == std::string("--trace_parsing"))
            driver.trace_parsing = true;
        else if (argv[i] == std::string("-s") || argv[i] == std::string("--trace_scanning"))
            driver.trace_scanning = true;
        else if (argv[i] == std::string("-emit-mir"))
            print_IR = true;
        else if (argv[i] == std::string("-emit-ast"))
            print_ast = true;
        else if (argv[i] == std::string("-nocheck"))
            check = false;
        else if (argv[i] == std::string("-o"))
            out_file = argv[++i];
        else{
            filename = argv[i];
        }
    }

    auto root = driver.parse(filename);
    if (print_ast)
        root->accept(printer);
    if(check){
        root->accept(checker);
        if(checker.is_err())
        {
            std::cout<<"The file has semantic errors\n";
            exit(-1);
        }
    }
    if (print_IR) {
        root->accept(builder);
        auto m = builder.getModule();
        m->set_print_name();
        auto IR = m->print();
        std::ofstream output_stream;
        output_stream.open(out_file, std::ios::out);
        output_stream << IR;
        output_stream.close();
    }
    return 0;
}
