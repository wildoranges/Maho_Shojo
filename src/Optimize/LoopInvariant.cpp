#include "LoopInvariant.h"

void LoopInvariant::execute(){
    CFG_analyser = std::make_unique<CFG_analyse>(module);
    CFG_analyser->execute();


    auto loops = CFG_analyser->get_loops();
    for (auto loop : *loops){
        bool inner = true;
        for (auto BB : *loop){
            if (CFG_analyser->find_bb_loop(BB) != loop){
                inner = false;
                break;
            }
        }
        if (inner == false){
            continue;
        }

        while (loop != nullptr){
            invariants.clear();
            invariants_find(loop);
            BasicBlock* pre_BB = nullptr;
            for (auto BB : CFG_analyser->find_loop_entry(loop)->get_pre_basic_blocks()){
                if (CFG_analyser->find_bb_loop(BB) != loop){
                    pre_BB = BB;
                    break;
                }
            }
            Instruction *terminator = pre_BB->get_terminator();
            pre_BB->get_instructions().pop_back();
            for (auto pair : invariants){
                for (auto inst : pair.second){
                    pair.first->get_instructions().remove(inst);
                    pre_BB->add_instruction(inst);
                }
            }
            pre_BB->add_instruction(terminator);
            loop = CFG_analyser->find_outer_loop(loop);
        }
    }
}

void LoopInvariant::invariants_find(std::vector<BasicBlock *>* loop){
    std::set<Value *> defined_in_loop;
    std::set<Instruction *> invariant;
    for (auto BB : *loop){
        for (auto inst : BB->get_instructions()){
            if (inst->is_store()){
                defined_in_loop.insert(inst->get_operand(1));
            }
            else{
                defined_in_loop.insert(inst);
            }
        }
    }
    bool change = false;
    do {
        change = false;
        for (auto BB : *loop){
            invariant.clear();
            for (auto inst : BB->get_instructions()){
                bool invariant_check = true;
                if (inst->is_call()||inst->is_alloca()||inst->is_ret()||inst->is_br()||inst->is_cmp()||inst->is_phi()||inst->is_load()){
                    continue;
                }
                if (defined_in_loop.find(inst) == defined_in_loop.end()){
                    continue;
                }
                for (auto operand : inst->get_operands()){
                    if (defined_in_loop.find(operand) != defined_in_loop.end()){
                        invariant_check = false;
                    }
                }
                if (invariant_check){
                    defined_in_loop.erase(inst);
                    invariant.insert(inst);
                    change = true;
                }
            }
            invariants.push_back({BB,invariant});
        }
    }while (change);
}