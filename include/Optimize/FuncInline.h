#ifndef MHSJ_FUNCINLINE_H
#define MHSJ_FUNCINLINE_H

#include "Pass.h"

class FuncInline : public Pass
{
private:
    /* data */
    std::vector<std::pair<Function*, std::pair<Instruction *,Function *>>> calling_pair;
public:
    explicit FuncInline(Module* module): Pass(module){}
    void execute() final;
    void no_recursive_find();
    void func_inline();
};




#endif
