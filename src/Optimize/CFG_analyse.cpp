#include "CFG_analyse.h"

void CFG_analyse::execute(){
    for (auto func : module->get_functions()){
        if (func->get_basic_blocks().size()==0) continue;
        incoming_find(func);
        loop_find(func);
    }
}

void CFG_analyse::incoming_find(Function* func){
    auto BB = func->get_entry_block();
    color[BB] = 1;
    incoming_DFS(BB);
}

void CFG_analyse::incoming_DFS(BasicBlock* BB){
    auto succs = BB->get_succ_basic_blocks();
    for (auto succ_BB : succs){
        if (color[succ_BB] == 0){
            succ_BB->incoming_add();
            color[BB] = 1;
            incoming_DFS(succ_BB);
        }
        else if (color[succ_BB] == 2){
            succ_BB->incoming_add();
        }
    }
    color[BB] = 2;
}

int DFN,LOW;
std::map<BasicBlock*,int> BB_DFN;
std::map<BasicBlock*,int> BB_LOW;
std::stack<BasicBlock*> BB_Stack;
std::vector<BasicBlock*>* BBs;
std::stack<std::vector<BasicBlock*>*> loop_stack;

void CFG_analyse::loop_find(Function *func){
    color.clear();
    DFN = 0;
    BB_DFN.clear();
    BB_LOW.clear();
    tarjan_DFS(func->get_entry_block());
    while(!loop_stack.empty()){
        auto loop = loop_stack.top();
        loop_stack.pop();
        if (bb_loop[find_loop_entry(loop)]!=nullptr){
            outer_loop[loop] = bb_loop[find_loop_entry(loop)];
        }
        for (auto BB : *loop){
            BB->loop_depth_add();
            bb_loop[BB] = loop;
        }
        color[find_loop_entry(loop)] = 1;
        DFN = 0;
        BB_DFN.clear();
        BB_LOW.clear();
        for (auto succ : find_loop_entry(loop)->get_succ_basic_blocks()){
            if (bb_loop[succ] == loop){
                tarjan_DFS(succ);
            }
        }
    }
}


void CFG_analyse::tarjan_DFS(BasicBlock *BB){
    DFN++;
    BB_DFN[BB] = DFN;
    BB_LOW[BB] = DFN;
    BB_Stack.push(BB);
    for (auto succ : BB->get_succ_basic_blocks()){
        if (color[succ] != 1){
            if (BB_DFN[succ] == 0){
                tarjan_DFS(succ);
            }
            if (BB_LOW[succ] < BB_LOW[BB]){
                BB_LOW[BB] = BB_LOW[succ];
            }
        }
    }
    if (BB_DFN[BB] == BB_LOW[BB]){
        BBs = new std::vector<BasicBlock*>;
        auto bb_instack = BB_Stack.top();
        while (BB_LOW[bb_instack] == BB_LOW[BB]){
            BB_Stack.pop();
            BBs->push_back(bb_instack);
            if (BB_Stack.empty()){
                break;
            }
            bb_instack = BB_Stack.top();
        }
        if (BBs->size()>1){
            loops.push_back(BBs);
            loop_stack.push(BBs);
        }
    }
}
