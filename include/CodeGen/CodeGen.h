#ifndef MHSJ_CODEGEN_H
#define MHSJ_CODEGEN_H

#include<Module.h>
#include<IR2asm.h>
#include<Value.h>
#include<Instruction.h>
#include<RegAlloc.h>

class CodeGen{
    const int int_align = 4;
    const int int_size = 4;
    const int int_p2align = 2;
    const int reg_size = 4;
    const int max_func_reg = 3;

    std::map<Value*, IR2asm::Regbase*> stack_map;
    std::vector<IR2asm::Regbase*> arg_on_stack;
    std::map<GlobalVariable *, IR2asm::label> global_variable_table;
    std::map<Function*, std::set<GlobalVariable *>> global_variable_use;
    std::pair<std::set<int>, std::set<int>> used_reg;
    std::map<Value*, Interval*>* reg_map;
    int func_no = 0;
    int bb_no = 0;
    int label_no = 0;
    int max_arg_size = 0;
    std::vector<BasicBlock*> linear_bb;
    std::map<BasicBlock*, IR2asm::label *> bb_label;
    bool have_func_call = true;
    std::map<int, std::vector<Value*>> reg2val;
    std::vector<int> to_save_reg;

public:
    void make_linear_bb(Function* fun);
    void func_call_check(Function* fun);

    std::string global(std::string name);
    bool iszeroinit(Constant * init);
    std::string module_gen(Module* module);
    std::string global_def_gen(Module* module);
    void make_global_table(Module* module);
    std::string function_gen(Function* function);
    int stack_space_allocation(Function* fun);
    std::string callee_reg_store(Function* fun);
    std::string callee_stack_operation_in(Function* fun, int stack_size);
    std::string arg_move(CallInst* call);
    std::string callee_reg_restore(Function* fun);
    std::string callee_stack_operation_out(Function* fun, int stack_size);
    std::string caller_reg_store(Function* fun,CallInst* call);
    std::string caller_reg_restore(Function* fun, CallInst* call);
    void global_label_gen(Function* fun);
    std::string print_global_table();
    std::string bb_gen(BasicBlock* bb);
    std::string instr_gen(Instruction * inst);
    std::string ret_mov(CallInst* call);
};

#endif