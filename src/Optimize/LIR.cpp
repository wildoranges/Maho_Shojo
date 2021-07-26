//
// Created by cjb on 7/23/21.
//

#include "LIR.h"
#include <cmath>

void LIR::execute() {
    for (auto func : module->get_functions()){
        if (func->get_num_basic_blocks()>0){
            for (auto bb : func->get_basic_blocks()){
                // split instr
                split_gep(bb);
                div_const2mul(bb);
                split_srem(bb);
                // convert instr
                // remove meaningless instr
                // merge instr
                merge_mul_add(bb);
                merge_mul_sub(bb);
                merge_cmp_br(bb);
            }
        }
    }
}

void LIR::merge_cmp_br(BasicBlock* bb) {
    auto terminator = bb->get_terminator();
    if (terminator->is_br()){
        auto br = dynamic_cast<BranchInst *>(terminator);
        if (br->is_cond_br()){
            auto inst = dynamic_cast<Instruction *>(br->get_operand(0));

            if (inst->is_cmp()) {
                auto br_operands = br->get_operands();
                auto inst_cmp = dynamic_cast<CmpInst *>(inst);
                if (inst_cmp->get_parent() == bb && inst_cmp->get_use_list().size() == 1) {
                    auto cmp_ops = inst_cmp->get_operands();
                    auto cmp_op = inst_cmp->get_cmp_op();
                    auto cmp_br = CmpBrInst::create_cmpbr(cmp_op,cmp_ops[0],cmp_ops[1],
                                                        dynamic_cast<BasicBlock* >(br_operands[1]),dynamic_cast<BasicBlock* >(br_operands[2]),
                                                        bb,module);
                    bb->delete_instr(inst_cmp);
                    bb->delete_instr(br);
                }
            }
        }
    }
}

void LIR::merge_mul_add(BasicBlock* bb) {
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin();iter != instructions.end();iter++){
        auto inst_add = *iter;
        if (inst_add->is_add()){
            auto op1 = inst_add->get_operand(0);
            auto op_ins1 = dynamic_cast<Instruction *>(op1);
            auto op2 = inst_add->get_operand(1);
            auto op_ins2 = dynamic_cast<Instruction *>(op2);
            if (op_ins1!=nullptr){
                if (op_ins1->is_mul() && op_ins1->get_parent() == bb && op_ins1->get_use_list().size() == 1){
                    auto mul_add = MulAddInst::create_muladd(op_ins1->get_operand(0),op_ins1->get_operand(1),op2,bb,module);
                    instructions.pop_back();
                    bb->add_instruction(iter,mul_add);
                    bb->delete_instr(op_ins1);
                    iter--;
                    inst_add->replace_all_use_with(mul_add);
                    bb->delete_instr(inst_add);
                    continue;
                }
            }
            if (op_ins2!=nullptr){
                if (op_ins2->is_mul() && op_ins2->get_parent() == bb && op_ins2->get_use_list().size() == 1){
                    auto mul_add = MulAddInst::create_muladd(op_ins2->get_operand(0),op_ins2->get_operand(1),op1,bb,module);
                    instructions.pop_back();
                    bb->add_instruction(iter,mul_add);
                    bb->delete_instr(op_ins2);
                    iter--;
                    inst_add->replace_all_use_with(mul_add);
                    bb->delete_instr(inst_add);
                    continue;
                }
            }
        }
    }
}

void LIR::merge_mul_sub(BasicBlock* bb) {
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin();iter != instructions.end();iter++){
        auto inst_sub = *iter;
        if (inst_sub->is_sub()){
            auto op1 = inst_sub->get_operand(0);
            auto op2 = inst_sub->get_operand(1);
            auto op_ins2 = dynamic_cast<Instruction *>(op2);
            if (op_ins2!=nullptr){
                if (op_ins2->is_mul() && op_ins2->get_parent() == bb && op_ins2->get_use_list().size() == 1){
                    auto mul_sub = MulSubInst::create_mulsub(op_ins2->get_operand(0),op_ins2->get_operand(1),op1,bb,module);
                    instructions.pop_back();
                    bb->add_instruction(iter,mul_sub);
                    bb->delete_instr(op_ins2);
                    iter--;
                    inst_sub->replace_all_use_with(mul_sub);
                    bb->delete_instr(inst_sub);
                    continue;
                }
            }
        }
    }
}

void LIR::split_gep(BasicBlock* bb) {
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++) {
        auto inst_gep = *iter;
        if (inst_gep->is_gep() && (inst_gep->get_num_operand() == 3)) {
            auto size = ConstantInt::get(inst_gep->get_type()->get_pointer_element_type()->get_size(), module);
            auto offset = inst_gep->get_operand(2);
            inst_gep->remove_use(offset);
            inst_gep->set_operand(2, ConstantInt::get(0, module));
            auto real_offset = BinaryInst::create_mul(offset, size, bb, module);
            bb->add_instruction(++iter, instructions.back());
            instructions.pop_back();
            auto real_ptr = BinaryInst::create_add(inst_gep, real_offset, bb, module);
            bb->add_instruction(iter--, instructions.back());
            instructions.pop_back();
            real_ptr->remove_use(inst_gep);
            inst_gep->replace_all_use_with(real_ptr);
            real_ptr->set_operand(0,inst_gep);
        }
    }
}

void LIR::split_srem(BasicBlock* bb) {
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin();iter != instructions.end();iter++){
        auto inst_rem = *iter;
        if (inst_rem->is_rem()){
            auto op1 = inst_rem->get_operand(0);
            auto op2 = inst_rem->get_operand(1);
            auto inst_div = BinaryInst::create_sdiv(op1,op2,bb,module);
            instructions.pop_back();
            auto inst_mul = BinaryInst::create_mul(inst_div,op2,bb,module);
            instructions.pop_back();
            auto inst_sub = BinaryInst::create_sub(op1,inst_mul,bb,module);
            instructions.pop_back();
            bb->add_instruction(iter,inst_div);
            bb->add_instruction(iter,inst_mul);
            bb->add_instruction(iter,inst_sub);
            inst_rem->replace_all_use_with(inst_sub);
            iter--;
            bb->delete_instr(inst_rem);
        }
    }
}

void LIR::mul_const2shift(BasicBlock* bb) {
    
}

void LIR::div_const2mul(BasicBlock* bb) {
    // FIXME: may have bugs, need many tests
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++) {
        auto instruction = *iter;
        if (instruction->is_div()) {
            auto op1 = instruction->get_operand(0);
            auto op2 = instruction->get_operand(1);
            auto op_const2 = dynamic_cast<ConstantInt*>(op2);
            if (op_const2 != nullptr) {
                auto divisor = op_const2->get_value();
                if (divisor == 0) {
                    std::cerr<<"divided by zero!"<<std::endl;
                    exit(-1);
                } else {
                    int abs_divisor = (divisor > 0) ? divisor : -divisor;
                    int c = 31 + floor(log2(abs_divisor));
                    long long L = pow(2, c);
                    int B = (divisor > 0) ? (floor(L / abs_divisor) + 1) : - (floor(L / abs_divisor) + 1);
                    iter++;
                    auto smul_lo = BinaryInst::create_smul_lo(op1, ConstantInt::get(B, module), bb, module);
                    bb->add_instruction(iter, instructions.back());
                    instructions.pop_back();
                    auto smul_hi = BinaryInst::create_smul_hi(op1, ConstantInt::get(B, module), bb, module);
                    bb->add_instruction(iter, instructions.back());
                    instructions.pop_back();
                    auto asr = BinaryInst::create_asr(smul_hi, ConstantInt::get(c - 32, module), bb, module);
                    bb->add_instruction(iter, instructions.back());
                    instructions.pop_back();
                    auto lsr = BinaryInst::create_lsr(smul_hi, ConstantInt::get(31, module), bb, module);
                    bb->add_instruction(iter, instructions.back());
                    instructions.pop_back();
                    auto add = BinaryInst::create_add(asr, lsr, bb, module);
                    bb->add_instruction(iter, instructions.back());
                    instructions.pop_back();
                    instruction->replace_all_use_with(add);
                    bb->delete_instr(instruction);
                    iter--;
                }
            }
        }
    }
}

void LIR::remove_unused_op(BasicBlock* bb) {
    // TODO: x+0, x-0, x-x, x*0, x*1, x/1, x or x, x and x, x xor x, x / x, x asr 0, x lsl 0, x lsr 0, and so on
}
