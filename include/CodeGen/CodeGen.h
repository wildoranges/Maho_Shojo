#ifndef MHSJ_CODEGEN_H
#define MHSJ_CODEGEN_H

#include<Module.h>
#include<IR2asm.h>
#include<Value.h>
#include<Instruction.h>

namespace CodeGen{

    std::map<Value*, int> stack_map;
    std::map<IR2asm::label, GlobalVariable *> global_variable_table;
    int func_no = 0;
    int bb_no = 0;
    int label_no = 0;
    std::vector<BasicBlock*> linear_bb;
    std::map<BasicBlock*, IR2asm::label> bb_label;

    void make_linear_bb(Function* fun);

    std::string global(std::string name);
    bool iszeroinit(Constant * init);
    std::string module_gen(Module* module);
    std::string global_def_gen(Module* module);
    std::string function_gen(Function* function);
    void stack_space_allocation(Function* fun);
    std::string callee_reg_store(Function* fun);
    std::string callee_stack_operation_in(Function* fun);
    std::string callee_reg_restore(Function* fun);
    std::string callee_stack_operation_out(Function* fun);
    std::string caller_reg_store(Function* fun);
    std::string caller_reg_restore(Function* fun);
    void make_global_table(Function *fun);
    std::string print_global_table(Function* fun);
    std::string bb_gen(BasicBlock* bb);
    std::string instr_gen(Instruction * inst);
}

#endif