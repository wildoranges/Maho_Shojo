#ifndef MHSJ_DEADCODEELIMINATION_H
#define MHSJ_DEADCODEELIMINATION_H

#include "Pass.h"
#include "Module.h"

class DeadCodeElimination : public Pass
{
public:
    DeadCodeElimination(Module *module) : Pass(module) {}
    void execute() final;
    void mark();
    void sweep();
    bool is_critical(Instruction *);
    BasicBlock* get_nearest_marked_postdominator(Instruction *);
    const std::string get_name() const override {return name;}
private:
    Function *func_;
    std::map<Instruction *, bool> instr_mark;
    std::string name = "DeadCodeElimination";
};


#endif  // MHSJ_DEADCODEELIMINATION_H
