#ifndef MHSJ_CFGSIMPLIFIER_H
#define MHSJ_CFGSIMPLIFIER_H

#include "Pass.h"
#include "Module.h"

class CFGSimplifier :public Pass{
public:
    explicit CFGSimplifier(Module *m) : Pass(m) {}
    virtual void execute() override final;
    void compute_postorder();
    bool one_pass();
    void combine_bb(BasicBlock*, BasicBlock*);
private:
    Function *func_;
    std::vector<BasicBlock*> postorder_bb_list;
};

#endif //MHSJ_CFGSIMPLIFIER_H
