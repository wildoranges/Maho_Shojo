#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

//TODO toposort
std::map<BasicBlock* , std::set<Instruction *>> invariant;
std::map<BBset_t *,int> hoistRecord;
std::map<BBset_t *,int> analyRecord;

void LoopInvHoist::findInv(BBset_t * loop)
{
    std::unordered_set<Value *> NoInvariantSet;
    std::set<Instruction *> ins_set;

    for(auto bb: *loop){
        for(auto ins : bb->get_instructions()){
            NoInvariantSet.insert(ins);
        }
    }//先将所有的指令放入非不变式集合

    bool NeedReScan = false;
    do{
        NeedReScan = false;
        for(auto bb : *loop){
            ins_set.clear();
            for(auto ins : bb->get_instructions()){
                bool isInv = true;
                if(ins->is_call()) 
                    continue;
                if(ins->is_alloca()) 
                    continue;
                if(ins->is_ret()) 
                    continue;
                if(ins->is_br()) 
                    continue;
                if(ins->is_cmp()) 
                    continue;
                if(ins->is_phi()) 
                    continue;
                if(NoInvariantSet.find(ins) == NoInvariantSet.end()) 
                    continue;
                for(auto val : ins->get_operands()){
                    if(NoInvariantSet.find(val) != NoInvariantSet.end()){
                        isInv = false;
                    }
                }//若存在数据的依赖关系，则目前分析中不是循环不变式。
                if(isInv){
                    NoInvariantSet.erase(ins);
                    ins_set.insert(ins);
                    NeedReScan = true;
                }
            }
            invariant.insert({bb,ins_set});
        }
    }while(NeedReScan);
}

void LoopInvHoist::hoistInv(BasicBlock *prebb)
{
    auto terminatorInstr = prebb->get_terminator();
    auto tmpInstr = terminatorInstr;
    prebb->delete_instr(terminatorInstr);

    for(auto iter = invariant.begin();iter != invariant.end();iter++){
        auto bb = iter->first;
        auto instrSet = iter->second;
        for(auto instr : instrSet){
            prebb->add_instruction(instr);
            bb->delete_instr(instr);
        }
    }

    prebb->add_instruction(tmpInstr);
    return;
}

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();
    
    // 接下来由你来补充啦！
    bool isHoist = false;
    for(auto func : m_->get_functions()){
        std::unordered_set<BBset_t *> loops_in_func = loop_searcher.get_loops_in_func(func);
        for(auto loop : loops_in_func){
            do{
                invariant.clear();
                isHoist = false;
                if(analyRecord[loop] == 1 && hoistRecord[loop] == 0) break;
                findInv(loop);
                if(!invariant.empty()){
                    auto base = loop_searcher.get_loop_base(loop);
                    for(auto prebb: base->get_pre_basic_blocks()){
                        if(loop_searcher.get_inner_loop(prebb) != loop)
                            hoistInv(prebb);
                    }
                    isHoist = true;
                    hoistRecord.insert({loop,0});
                    analyRecord.insert({loop,1});
                }
                loop = loop_searcher.get_parent_loop(loop);
                if(loop != nullptr) hoistRecord.insert({loop,isHoist});
            }while(loop);
        }
    }
}