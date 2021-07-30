#ifndef MHSJ_CFGSIMPLIFIER_H
#define MHSJ_CFGSIMPLIFIER_H

#include "Pass.h"
#include "Module.h"

class CFGSimplifier :public Pass{
public:
    explicit CFGSimplifier(Module *m) : Pass(m) {}
    void execute() override final;
    const std::string get_name() const override {return name;}
    void compute_postorder();
    bool one_pass();
    void combine_bb(BasicBlock*, BasicBlock*);
    bool bb_can_delete(BasicBlock*);
    void replace_phi(BasicBlock*, std::list<BasicBlock*>, BasicBlock*);
private:
    Function *func_;
    std::vector<BasicBlock*> postorder_bb_list;
    std::string name = "CFGSimplifier";
};

#endif //MHSJ_CFGSIMPLIFIER_H