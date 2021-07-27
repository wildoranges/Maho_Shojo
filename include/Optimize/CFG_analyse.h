#ifndef MHSJ_CFG_ANALYSE_H
#define MHSJ_CFG_ANALYSE_H

#include "Module.h"
#include "Pass.h"
#include <vector>
#include <map>
#include <stack>

/***************searching incoming forwarding branches****************/
/***************searching loops****************/


class CFG_analyse : public Pass{
private:
    std::vector<std::vector<BasicBlock *>*> loops;

    std::map<BasicBlock *,std::vector<BasicBlock *>*> bb_loop;

    std::map<BasicBlock*,int>color;
    


public:
    explicit CFG_analyse(Module* module): Pass(module){}
    void execute() final;
    void incoming_find(Function* func);
    void incoming_DFS(BasicBlock* BB);
    void loop_find(Function* func);
    void tarjan_DFS(BasicBlock* BB);
    BasicBlock* loop_entry(std::vector<BasicBlock *>* loop);
    std::vector<BasicBlock *>* find_bb_loop(BasicBlock* BB);
};





#endif