//
// Created by cjb on 7/23/21.
//

#include "LIR.h"

void LIR::execute() {
    for (auto func : module->get_functions()){
        if (func->get_num_basic_blocks()>0){
            for (auto bb : func->get_basic_blocks()){
                merge_cmp_br(bb);
                merge_mul_add(bb);
            }
        }
        
    }
}

void LIR::merge_cmp_br(BasicBlock* bb) {

}

void LIR::merge_mul_add(BasicBlock* bb) {
    auto &instructions = bb->get_instructions();
    std::cout<<instructions.size()<<std::endl;
    for (auto iter = instructions.begin();iter != instructions.end();iter++){
        auto instruction = *iter;
        if (instruction->is_add()){
            auto op1 = instruction->get_operand(0);
            auto op_ins1 = dynamic_cast<Instruction *>(op1);
            auto op2 = instruction->get_operand(1);
            auto op_ins2 = dynamic_cast<Instruction *>(op2);
            if (op_ins1!=nullptr){
                if (op_ins1->is_mul() && op_ins1->get_parent() == bb && op_ins1->get_use_list().size() == 1){
                    auto mul_add = MulAddInst::create_muladd(op2,op_ins1->get_operand(0),op_ins1->get_operand(1),bb,module);
                    bb->delete_instr(mul_add);
                    bb->add_instruction(iter,mul_add);
                    bb->delete_instr(op_ins1);
                    iter--;
                    bb->delete_instr(instruction);
                    continue;
                }
            }
            if (op_ins2!=nullptr){
                if (op_ins2->is_mul() && op_ins2->get_parent() == bb && op_ins2->get_use_list().size() == 1){
                    auto mul_add = MulAddInst::create_muladd(op1,op_ins2->get_operand(0),op_ins2->get_operand(1),bb,module);
                    bb->delete_instr(mul_add);
                    bb->add_instruction(iter,mul_add);
                    bb->delete_instr(op_ins2);
                    iter--;
                    bb->delete_instr(instruction);
                    continue;
                }
            }
        }
    }
}

void LIR::merge_mul_sub(BasicBlock* bb) {
    
}

void LIR::split_gep(BasicBlock* bb) {
    
}

void LIR::split_srem(BasicBlock* bb) {
    
}

void LIR::mul_const2shift(BasicBlock* bb) {
    
}

void LIR::div_const2shift(BasicBlock* bb) {
    
}

void LIR::remove_unused_op(BasicBlock* bb) {
    
}
