#include "mem2reg.h"
#include "IRBuilder.h"

void Mem2Reg::execute(){
    for(auto fun: module->get_functions()){
        func_ = fun;
        insideBlockForwarding();
        // genPhi();
        // valueForwarding(func_->get_entry_block());
    }
}

void Mem2Reg::insideBlockForwarding(){
    for(auto bb: func_->get_basic_blocks()){
        std::set<Value *> defined_list;
        std::map<std::pair<Value *, Value *>, std::set<Instruction *>> forward_list;
        std::map<Value *, Value *> new_value;
        for(auto inst: bb->get_instructions()){
            if(!isLocalVarOp(inst))continue;
            if(inst->get_instr_type() == Instruction::OpID::store){
                Value* lvalue = static_cast<StoreInst *>(inst)->get_lval();
                Value* rvalue = static_cast<StoreInst *>(inst)->get_rval();
                defined_list.insert(lvalue);
                if(new_value.find(lvalue) != new_value.end()){
                    new_value.find(lvalue)->second = rvalue;
                }
                else{
                    new_value.insert({lvalue, rvalue});
                }
            }
            else if(inst->get_instr_type() == Instruction::OpID::load){
                Value* lvalue = static_cast<LoadInst *>(inst)->get_lval();
                if(defined_list.find(lvalue) != defined_list.end())continue;
                Value* value = new_value.find(lvalue)->second;
                if(forward_list.find({lvalue, value}) != forward_list.end()){
                    forward_list.find({lvalue, value})->second.insert(inst);
                }
                else{
                    forward_list.insert({{lvalue, value},{inst}});
                }
            }
        }

        for(auto submap: forward_list){
            Value * lvalue = submap.first.first;
            Value * value = submap.first.second;
            for(Instruction* inst: submap.second){
                for(auto use: inst->get_use_list()){
                    Instruction * use_inst = dynamic_cast<Instruction *>(use.val_);
                    use_inst->set_operand(use.arg_no_, value);
                }
                bb->delete_instr(inst);
            }
        }        
    }
}

void Mem2Reg::genPhi(){
    std::set<Value *> globals;
    for(auto bb: func_->get_basic_blocks()){
        std::set<Value *> defined_list;
        for(auto inst: bb->get_instructions()){
            if(inst->get_instr_type() == Instruction::OpID::load){
                if(!isLocalVarOp(inst))continue;
                Value* lvalue = static_cast<LoadInst *>(inst)->get_lval();
                if(defined_list.find(lvalue) != defined_list.end())continue;
            }
        }
    }
}


