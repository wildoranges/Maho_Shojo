#ifndef MHSJ_SCCP_H
#define MHSJ_SCCP_H

#include "Pass.h"
#include "Module.h"
#include "ActiveVar.h"

enum Mark {
    unexecuted,
    executed
};

enum LatticeValue {
    top,
    bottom,
    other
};

class SCCP : public Pass
{
public:
    SCCP(Module *module) : Pass(module) {}
    void execute() final;
    void evaluate_assign();
    void evaluate_conditional();
    void evaluate_phi();
    void evaluate_all_phis_in_block(std::pair<BasicBlock*, BasicBlock*>);
    void evaluate_operands();
    void evaluate_result();
    const std::string get_name() const override {return name;}
private:
    Function *func_;
    std::vector<std::pair<BasicBlock*, BasicBlock*>> CFGWorkList, SSAWorkList;
    std::map<std::pair<BasicBlock*, BasicBlock*>, Mark> CFGWorkList_mark;
    std::map<Value*, LatticeValue> lattice_value_map;
    std::string name = "SCCP";
};

#endif  // MHSJ_SCCP_H
