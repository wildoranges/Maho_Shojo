#ifndef MHSJ_CODEGEN_H
#define MHSJ_CODEGEN_H

#include<Module.h>
#include<IR2asm.h>
#include<Value.h>
#include<Instruction.h>
#include<RegAlloc.h>

namespace CodeGen{
    const int int_align = 4;
    const int int_size = 4;
    const int int_p2align = 2;
    const int reg_size = 4;

    std::map<Value*, IR2asm::Regbase*> stack_map;
    std::map<GlobalVariable *, IR2asm::label> global_variable_table;
    std::map<Function*, std::set<GlobalVariable *>> global_variable_use;
    std::set<int> used_reg;
    std::map<Value*, Interval*>& reg_map;
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
    void make_global_table(Module* module);
    std::string function_gen(Function* function);
    int stack_space_allocation(Function* fun, RegAllocDriver* driver);
    std::string callee_reg_store(Function* fun);
    std::string callee_stack_operation_in(Function* fun, int stack_size);
    std::string callee_reg_restore(Function* fun);
    std::string callee_stack_operation_out(Function* fun, int stack_size);
    std::string caller_reg_store(Function* fun);
    std::string caller_reg_restore(Function* fun);
    void global_label_gen(Function* fun);
    std::string print_global_table();
    std::string bb_gen(BasicBlock* bb);
    std::string instr_gen(Instruction * inst);
}

#endif