#include "FuncInline.h"

#define HUGE_SIZE 100

void FuncInline::execute(){
    no_recursive_call_find();
    func_inline();
}

void FuncInline::no_recursive_call_find(){
    std::map<Function* ,int> recursive_check;
    for (auto func : module->get_functions()){
        if (func->get_num_basic_blocks() == 0){
            continue;
        }
        auto LOC = 0;
        for (auto BB : func->get_basic_blocks()){
            LOC += BB->get_num_of_instr();
        }
        if (LOC > HUGE_SIZE){
            continue;
        }
        auto use_list = func->get_use_list();
        for (auto use : use_list){
            auto inst_val = use.val_;
            auto inst = dynamic_cast<Instruction *>(inst_val);
            if (inst->get_parent()->get_parent() == func){
                recursive_check[func] = 1;
                break;
            }
        }
        if (recursive_check[func] == 1){
            continue;
        }
        for (auto use : use_list){
            auto inst_val = use.val_;
            auto inst = dynamic_cast<Instruction *>(inst_val);
            calling_pair.push_back({inst->get_parent()->get_parent(),{inst,func}});
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
        for (auto iter_inst = call_BB->get_instructions().begin(); iter_inst!=call_BB->get_instructions().end(); iter_inst++){
            if (need_move){
                auto inst = *iter_inst;
                split_BB->add_instruction(inst);
                inst->set_parent(split_BB);
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
            for (auto succ_inst : succ_BB->get_instructions()){
                if (succ_inst->is_phi()){
                    for (auto i = 0; i < succ_inst->get_num_operand(); i=i+2){
                        auto from_BB = dynamic_cast<BasicBlock*>(succ_inst->get_operand(i+1));
                        if (from_BB == call_BB){
                            from_BB->remove_use(succ_inst);
                            succ_inst->set_operand(i + 1, split_BB);
                        }
                    }
                }
            }
        }
        call_BB->get_succ_basic_blocks().clear();
        //relation of split_BB has been maintained
        //call_BB have a call instruction as end
        //now I need to copy the BBs of call_func
        auto call_func_BBs = call_func->get_basic_blocks();
        BasicBlock *new_BB;
        Instruction *new_inst;
        std::map<Value*,Value*>old2new_inst;
        std::map<Value*,BasicBlock*>old2new_BB;
        std::vector<BasicBlock*>new_BBs;
        std::vector<std::pair<Value*, BasicBlock*>>split_phi_pair;
        int arg_idx = 1;
        for (auto iter_arg = call_func->get_args().begin(); iter_arg != call_func->get_args().end(); iter_arg++){
            auto arg = *iter_arg;
            old2new_inst[arg] = inst_call->get_operand(arg_idx++);
        }
        for (auto old_BB : call_func_BBs){
            new_BB = BasicBlock::create(module,"",func);
            old2new_BB[old_BB] = new_BB;
            new_BBs.push_back(new_BB);
            for (auto old_inst : old_BB->get_instructions()){
                if (old_inst->is_phi()){
                    new_inst = PhiInst::create_phi(old_inst->get_type(),new_BB);
                    new_BB->add_instruction(new_inst);
                    for (auto op : old_inst->get_operands()){
                        new_inst->add_operand(op);
                    }
                }
                else{
                    new_inst = old_inst->copy_inst(new_BB);
                    //not Value* operand
                    if (new_inst->is_cmpbr()){
                        new_inst->set_operand(2,old_inst->get_operand(2));
                        new_inst->set_operand(3,old_inst->get_operand(3));
                    }
                    if (new_inst->is_br()){
                        if (new_inst->get_num_operand() == 1){
                            new_inst->set_operand(0,old_inst->get_operand(0));
                        }
                        else{
                            new_inst->set_operand(1,old_inst->get_operand(1));
                            new_inst->set_operand(2,old_inst->get_operand(2));
                        }
                    }
                    if (new_inst->is_call()){
                        new_inst->set_operand(0,old_inst->get_operand(0));
                    }
                }
                old2new_inst[old_inst] = new_inst;
            }
        }
        for (auto BB : new_BBs){
            for (auto inst : BB->get_instructions()){
                if (inst->is_phi()){
                    for (auto i = 0; i < inst->get_num_operand(); i=i+2){
                        if(old2new_inst[inst->get_operand(i)]!=nullptr){
                            inst->get_operand(i)->remove_use(inst);
                            inst->set_operand(i,old2new_inst[inst->get_operand(i)]);
                        }
                        inst->get_operand(i+1)->remove_use(inst);
                        inst->set_operand(i+1,old2new_BB[inst->get_operand(i+1)]);
                    }
                }
                else if(inst->is_br()){
                    if(inst->get_num_operand() == 3){
                        if(old2new_inst[inst->get_operand(0)]!=nullptr){
                            inst->get_operand(0)->remove_use(inst);
                            inst->set_operand(0,old2new_inst[inst->get_operand(0)]);
                        }
                        inst->get_operand(1)->remove_use(inst);
                        auto true_BB = old2new_BB[inst->get_operand(1)];
                        inst->set_operand(1,true_BB);
                        true_BB->add_pre_basic_block(BB);
                        BB->add_succ_basic_block(true_BB);
                        auto false_BB = old2new_BB[inst->get_operand(2)];
                        inst->get_operand(2)->remove_use(inst);
                        inst->set_operand(2,false_BB);
                        false_BB->add_pre_basic_block(BB);
                        BB->add_succ_basic_block(false_BB);
                    }
                    else{
                        inst->get_operand(0)->remove_use(inst);
                        auto true_BB = old2new_BB[inst->get_operand(0)];
                        inst->set_operand(0,true_BB);
                        true_BB->add_pre_basic_block(BB);
                        BB->add_succ_basic_block(true_BB);
                    }
                }
                else if(inst->is_ret()){
                    if (inst->get_num_operand()>0){
                        split_phi_pair.push_back({inst->get_operand(0),BB});
                    }
                    BB->delete_instr(inst);
                    BranchInst::create_br(split_BB,BB);
                    break;
                }
                else{
                    for (auto i = 0; i < inst->get_num_operand(); i++){
                        if(old2new_inst[inst->get_operand(i)]!=nullptr){
                            inst->get_operand(i)->remove_use(inst);
                            inst->set_operand(i,old2new_inst[inst->get_operand(i)]);
                        }
                    }
                }
            }
        }
        //then remove the inst_call and new a br to new BB
        if (split_phi_pair.size()>1){
            auto ret_phi = PhiInst::create_phi(call_func->get_return_type(), split_BB);
            split_BB->add_instr_begin(ret_phi);
            for (auto phi_pair : split_phi_pair){
                Value *real_val;
                if (old2new_inst[phi_pair.first]==nullptr){
                    real_val = phi_pair.first;
                }
                else{
                    real_val = old2new_inst[phi_pair.first];
                }
                auto real_BB = phi_pair.second;
                ret_phi->add_phi_pair_operand(real_val,real_BB);
            }
            inst_call->replace_all_use_with(ret_phi);
        }
        if (split_phi_pair.size()==1){
            auto ret_val = (*split_phi_pair.begin()).first;
            if (old2new_inst[ret_val]!=nullptr){
                ret_val = old2new_inst[ret_val];
            }
            inst_call->replace_all_use_with(ret_val);
        }
        call_BB->delete_instr(inst_call);
        auto new_entry = old2new_BB[call_func->get_entry_block()];
        BranchInst::create_br(new_entry, call_BB);
    }
}