#include "mem2reg.h"
#include "IRBuilder.h"

void Mem2Reg::execute(){
    for(auto fun: module->get_functions()){
        if(fun->get_basic_blocks().size()==0)continue;
        func_ = fun;
        insideBlockForwarding();
        genPhi();
        module->set_print_name();
        valueDefineCounting();
        //std::cout << "VDC\n";
        valueForwarding(func_->get_entry_block());
    }
}

void Mem2Reg::insideBlockForwarding(){
    for(auto bb: func_->get_basic_blocks()){
        std::map<Value *, Instruction *> defined_list;
        std::map<std::pair<Value *, Value *>, std::set<Instruction *>> forward_list;
        std::map<Value *, Value *> new_value;
        std::set<Instruction *> delete_list;
        for(auto inst: bb->get_instructions()){
            if(!isLocalVarOp(inst))continue;
            if(inst->get_instr_type() == Instruction::OpID::store){
                Value* lvalue = static_cast<StoreInst *>(inst)->get_lval();
                Value* rvalue = static_cast<StoreInst *>(inst)->get_rval();
                // defined_list.insert(lvalue);
                if(defined_list.find(lvalue) != defined_list.end()){
                    auto pair = defined_list.find(lvalue);
                    delete_list.insert(pair->second);
                    pair->second = inst;
                }
                else{
                    defined_list.insert({lvalue, inst});
                }
                if(new_value.find(lvalue) != new_value.end()){
                    new_value.find(lvalue)->second = rvalue;
                }
                else{
                    new_value.insert({lvalue, rvalue});
                }
            }
            else if(inst->get_instr_type() == Instruction::OpID::load){
                Value* lvalue = static_cast<LoadInst *>(inst)->get_lval();
                if(defined_list.find(lvalue) == defined_list.end())continue;
                Value* value = new_value.find(lvalue)->second;
                if(forward_list.find({lvalue, value}) != forward_list.end()){
                    forward_list.find({lvalue, value})->second.insert(inst);
                }
                else{
                    forward_list.insert({{lvalue, value},{inst}});
                }
            }
        }
        // //debug
        // //std::cout << "new_value:\n";
        // for(auto p: new_value){
        //     //std::cout << p.first->get_name() << "<<>>" << p.second->get_name() << "\n";
        // }
        // //std::cout << "forwardlist:\n";
        // for(auto submap: forward_list){
        //     //std::cout << submap.first.first->get_name() << " " << submap.first.second->get_name() << "\n\t";
        //     for(auto inst: submap.second){
        //         //std::cout << inst->get_name() << " ";
        //     }
        //     //std::cout << "\n";
        // }
        // //std::cout << "\n";

        for(auto submap: forward_list){
            // Value * lvalue = submap.first.first;
            Value * value = submap.first.second;
            for(Instruction* inst: submap.second){
                for(auto use: inst->get_use_list()){
                    Instruction * use_inst = dynamic_cast<Instruction *>(use.val_);
                    use_inst->set_operand(use.arg_no_, value);
                }
                bb->delete_instr(inst);
            }
        } 
        for(auto inst:delete_list){
            bb->delete_instr(inst);
        }       
    }
}

void Mem2Reg::genPhi(){
    std::set<Value *> globals;
    std::map<Value *, std::set<BasicBlock *>> defined_in_block;
    for(auto bb: func_->get_basic_blocks()){
        for(auto inst: bb->get_instructions()){
            if(!isLocalVarOp(inst))continue;
            if(inst->get_instr_type() == Instruction::OpID::load){
                Value* lvalue = static_cast<LoadInst *>(inst)->get_lval();
                globals.insert(lvalue);
            }
            else if(inst->get_instr_type() == Instruction::OpID::store){
                Value* lvalue = static_cast<StoreInst *>(inst)->get_lval();
                if(defined_in_block.find(lvalue) != defined_in_block.end()){
                    defined_in_block.find(lvalue)->second.insert(bb);
                }
                else{
                    defined_in_block.insert({lvalue, {bb}});
                }
            }
        }
    }

    std::map<BasicBlock *, std::set<Value *>> bb_phi_list;

    for(auto var: globals){
        //std::cout << var->get_name() << "\n";
        auto define_bbs = defined_in_block.find(var)->second;
        std::vector<BasicBlock *> queue;
        queue.assign(define_bbs.begin(), define_bbs.end());
        int iter_pointer = 0;
        for(; iter_pointer < queue.size(); iter_pointer++){
            for(auto bb_domfront: queue[iter_pointer]->get_dom_frontier()){
                if(bb_phi_list.find(bb_domfront) != bb_phi_list.end()){
                    auto phis = bb_phi_list.find(bb_domfront);
                    if(phis->second.find(var) == phis->second.end()){
                        phis->second.insert(var);
                        auto newphi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), 
                            bb_domfront);
                        newphi->set_lval(var);
                        bb_domfront->add_instr_begin(newphi);
                        queue.push_back(bb_domfront);
                    }
                }
                else{
                    auto newphi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), 
                            bb_domfront);
                    newphi->set_lval(var);
                    bb_domfront->add_instr_begin(newphi);                  
                    queue.push_back(bb_domfront);
                    bb_phi_list.insert({bb_domfront, {var}});
                }
            }
        }
    }
}

void Mem2Reg::valueDefineCounting(){
    define_var = std::map<BasicBlock *, std::vector<Value*>>();
    for(auto bb: func_->get_basic_blocks()){
        define_var.insert({bb, {}});
        auto var_set = define_var.find(bb)->second;
        for(auto inst: bb->get_instructions()){
            if(inst->get_instr_type() == Instruction::OpID::phi){
                auto lvalue = dynamic_cast<PhiInst *>(inst)->get_lval();
                var_set.push_back(lvalue);
            }
            else if(inst->get_instr_type() == Instruction::OpID::store){
                if(!isLocalVarOp(inst))continue;
                auto lvalue = dynamic_cast<StoreInst *>(inst)->get_lval();
                var_set.push_back(lvalue);
            }
        }
    }
}

std::map<Value *, std::vector<Value *>> value_status;

void Mem2Reg::valueForwarding(BasicBlock* bb){
    //std::cout << bb->get_name() << "\n";
    std::set<Instruction *> delete_list;
    for(auto inst: bb->get_instructions()){
        if(inst->get_instr_type() != Instruction::OpID::phi)break;
        auto lvalue = dynamic_cast<PhiInst *>(inst)->get_lval();
        auto value_list = value_status.find(lvalue);
        if(value_list != value_status.end()){
            value_list->second.push_back(inst);
        }
        else{
            value_status.insert({lvalue, {inst}});
        }
    }

    for(auto inst: bb->get_instructions()){
        if(inst->get_instr_type() == Instruction::OpID::phi)continue;
        if(!isLocalVarOp(inst))continue;
        if(inst->get_instr_type() == Instruction::OpID::load){
            Value* lvalue = static_cast<LoadInst *>(inst)->get_lval();
            Value* new_value = *(value_status.find(lvalue)->second.end() - 1);
            for(auto use: inst->get_use_list()){
                auto user_inst = dynamic_cast<Instruction *>(use.val_);
                user_inst->set_operand(use.arg_no_, new_value);
            }
        }
        else if(inst->get_instr_type() == Instruction::OpID::store){
            Value* lvalue = static_cast<StoreInst *>(inst)->get_lval();
            Value* rvalue = static_cast<StoreInst *>(inst)->get_rval();
            if(value_status.find(lvalue) != value_status.end()){
                value_status.find(lvalue)->second.push_back(rvalue);
            }
            else{
                value_status.insert({lvalue, {rvalue}});
            }
        }
        delete_list.insert(inst);
    }

    for(auto succbb: bb->get_succ_basic_blocks()){
        for(auto inst: succbb->get_instructions()){
            if(inst->get_instr_type() == Instruction::OpID::phi){
                auto phi = dynamic_cast<PhiInst *>(inst);
                auto lvalue = phi->get_lval();
                if(value_status.find(lvalue) != value_status.end()){
                    if(value_status.find(lvalue)->second.size() > 0){
                        Value * new_value = *(value_status.find(lvalue)->second.end() - 1);
                        phi->add_phi_pair_operand(new_value, bb);
                    }
                    else{
                        //std::cout << "undefined value used: " << lvalue->get_name() << "\n";
                        // exit(-1);
                    }
                }
                else{
                    //std::cout << "undefined value used: " << lvalue->get_name() << "\n";
                    // exit(-1);
                }
            }
        }
    }

    for(auto succbb: bb->get_succ_basic_blocks()){
        valueForwarding(succbb);
    }

    // for(auto inst: bb->get_instructions()){
        auto var_set = define_var.find(bb)->second;
        for(auto var: var_set){
            if(value_status.find(var) == value_status.end())continue;
            if(value_status.find(var)->second.size() == 0)continue;
            value_status.find(var)->second.pop_back();
        }
    // }

    for(auto inst: delete_list){
        bb->delete_instr(inst);
    }
} 

