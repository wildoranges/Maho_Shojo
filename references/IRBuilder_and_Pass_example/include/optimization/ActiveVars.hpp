#ifndef ACTIVEVARS_HPP
#define ACTIVEVARS_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <queue>
#include <fstream>

ConstantFP* cast_constantfp(Value *value);
ConstantInt* cast_constantint(Value *value);

class ActiveVars : public Pass
{
public:
    ActiveVars(Module *m) : Pass(m) {}
    void run();
    void get_def_vars(Function *);
    void get_use_vars(Function *);
    std::string print();
private:
    Function *func_;
    std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
    std::map<BasicBlock *, std::set<Value *>> use_vars, def_vars;
};

#endif