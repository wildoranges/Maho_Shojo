#ifndef MHSJ_ACTIVEVAR_H
#define MHSJ_ACTIVEVAR_H

#include "Pass.h"
#include "Module.h"

#include <set>
#include <map>

class ActiveVar : public Pass
{
public:
    ActiveVar(Module *module) : Pass(module) {}
    void execute() final;
    void get_def_var();
    void get_use_var();
    void get_live_in_live_out();
private:
    Function *func_;
    std::map<BasicBlock *, std::set<Value *>> use_var, def_var;
    std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
};

#endif  // MHSJ_ACTIVEVAR_H
