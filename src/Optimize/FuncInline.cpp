#include "FuncInline.h"

#define HUGE_SIZE 10000

void FuncInline::execute(){

}

void FuncInline::no_recursive_find(){
    std::map<Function* ,int> recursive_check;
    for (auto func : module->get_functions()){
        for (auto call_func : func->get_calling_func()){
            if (call_func.second == func){
                recursive_check[func] == 1;
                break;
            }
        }
    }
    for (auto func : module->get_functions()){
        for (auto call_func : func->get_calling_func()){
            if (recursive_check[func] == 1){
                continue;
            }
            calling_pair.push_back({func,call_func});
        }
    }
}

void FuncInline::func_inline(){
    for (auto pair : calling_pair){
        auto func = pair.first;
        auto inst_call = pair.second.first;
        auto call_func = pair.second.second;

        auto call_BB = inst_call->get_parent();
        auto split_BB = BasicBlock::create(module,"",func);
        bool need_move = false;
        //split BasicBlock of inst_call
        for (auto iter_inst = call_BB->get_instructions().begin(); iter_inst!=call_BB->get_instructions().end(); iter_inst++){
            if (need_move){
                auto inst = *iter_inst;
                split_BB->add_instruction(inst);
                inst->set_parent(split_BB);
                for (auto use : (*iter_inst)->get_use_list()){
                    auto use_inst = dynamic_cast<Instruction *>(use.val_);
                    if (use_inst->is_phi()){
                        auto from_BB = dynamic_cast<BasicBlock *>(use_inst->get_operand(use.arg_no_ + 1));
                        if (from_BB == call_BB){
                            from_BB->remove_use(use_inst->get_operand(use.arg_no_ + 1));
                            use_inst->set_operand(use.arg_no_ + 1, split_BB);
                        }
                    }
                }
                iter_inst--;
                call_BB->get_instructions().remove(inst);
            }
            if (*iter_inst == inst_call){
                need_move = true;
            }
        }
        for (auto succ_BB : call_BB->get_succ_basic_blocks()){
            succ_BB->remove_pre_basic_block(call_BB);
            succ_BB->add_pre_basic_block(split_BB);
            split_BB->add_succ_basic_block(succ_BB);
        }
        call_BB->get_succ_basic_blocks().clear();
        //relation of split_BB has been maintained
        //call_BB have a call instruction as end
        //now I need to copy the BBs of call_func
        auto call_func_BBs = call_func->get_basic_blocks();
        BasicBlock *new_BB;
        Instruction *new_inst;
        std::map<Instruction*,Instruction*>old2new_inst;
        std::map<BasicBlock*,BasicBlock*>old2new_BB;
        for (auto old_BB : call_func_BBs){
            new_BB = BasicBlock::create(module,"",func);
            for (auto old_inst : old_BB->get_instructions()){
                new_inst = old_inst->copy(new_BB);
                //TODO
            }
        }
        

    }
}