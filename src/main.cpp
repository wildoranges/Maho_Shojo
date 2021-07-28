#include <iostream>
#include "MHSJBuilder.hpp"
#include "MHSJDriver.h"
#include "SyntaxTreePrinter.h"
#include "SyntaxTreeChecker.h"

#include "Pass.h"
#include "DominateTree.h"
#include "mem2reg.h"
#include "LIR.h"
#include "ActiveVar.h"
#include "ConstPropagation.h"
#include "DeadCodeElimination.h"
#include "CFGSimplifier.h"
#include "CFG_analyse.h"

void print_help(const std::string& exe_name) {
  std::cout << "Usage: " << exe_name
            << " [ -h | --help ] [ -p | --trace_parsing ] [ -s | --trace_scanning ] [ -emit-mir ] [ -emit-ast ] [-nocheck] [-o <output-file>] "
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
    std::string filename = "test.sy";
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
#ifdef DEBUG
        std::cout << "module\n";
#endif
        PassMgr passmgr(m.get());
#ifdef DEBUG
        std::cout << "passmgr\n";
#endif
        passmgr.addPass<DominateTree>();
        passmgr.addPass<RDominateTree>();
#ifdef DEBUG
        std::cout << "DomTree\n";
#endif
        passmgr.addPass<Mem2Reg>();
#ifdef DEBUG
        std::cout << "Mem2Reg\n";
#endif
        //passmgr.addPass<LIR>();
        passmgr.addPass<ActiveVar>();
        passmgr.addPass<ConstPropagation>();
        passmgr.addPass<DeadCodeElimination>();
        passmgr.addPass<CFGSimplifier>();
        passmgr.addPass<CFG_analyse>();
        m->set_print_name();
        passmgr.execute();
#ifdef DEBUG
        std::cout << "exec\n";
#endif
        m->set_print_name();
#ifdef DEBUG
        std::cout << "setname\n";
#endif
        auto IR = m->print();
#ifdef DEBUG
        std::cout << "prtm\n";
#endif
        std::ofstream output_stream;
        output_stream.open(out_file, std::ios::out);
        output_stream << IR;
        //std::cout << "outputir\n";
        output_stream.close();
    }
    return 0;
}
