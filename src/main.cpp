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
#include "CodeGen.h"

#include "LoopInvariant.h"
#include "AvailableExpr.h"


void print_help(const std::string& exe_name) {
  std::cout << "Usage: " << exe_name
            << " [ -h | --help ] [ -p | --trace_parsing ] [ -s | --trace_scanning ] [ -emit-mir ] [ -emit-ast ] [-nocheck] [-o <output-file>] [ -O0 ] [ -S ]"
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
    bool codegen=false;
    bool no_optimize = false;
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
        else if (argv[i] == std::string("-O0")) {
            no_optimize = true;
        }
        else if (argv[i] == std::string("-S")){
            codegen = true;
            print_IR = true;
        }
        else {
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
        if (no_optimize == true) {
            PassMgr passmgr(m.get());
            passmgr.addPass<DominateTree>();
            passmgr.addPass<Mem2Reg>();
            passmgr.execute();
        } else {
            PassMgr passmgr(m.get());

            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<DeadCodeElimination>();

            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<DominateTree>();
            passmgr.addPass<Mem2Reg>();
            passmgr.addPass<DeadCodeElimination>();

            passmgr.addPass<ConstPropagation>();
            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();
            passmgr.addPass<AvailableExpr>();

            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<CFGSimplifier>();
            passmgr.addPass<ConstPropagation>();
            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<LoopInvariant>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<AvailableExpr>();
            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<CFGSimplifier>();

            passmgr.addPass<DeadCodeElimination>();

            //passmgr.addPass<LIR>();
            passmgr.addPass<DeadCodeElimination>();
            passmgr.addPass<AvailableExpr>();
            passmgr.addPass<DeadCodeElimination>();

            passmgr.addPass<ActiveVar>();
            passmgr.addPass<CFG_analyse>();
            /****passmgr.addPass<CFG_analyse>();****
             ***this is executed in LoopInvariant***/
            m->set_print_name();
            passmgr.execute();
        }

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
        if(codegen){
            CodeGen coder = CodeGen();
            auto asmcode = coder.module_gen(m.get());
            std::ofstream output_stream;
            output_stream.open(out_file, std::ios::out);
            output_stream << asmcode;
            output_stream.close();
        }
        else{
            std::ofstream output_stream;
            output_stream.open(out_file, std::ios::out);
            output_stream << IR;
            //std::cout << "outputir\n";
            output_stream.close();
        }
    }
    
    return 0;
}
