#include "ConstPropagation.h"

ConstantInt *ConstFolder::compute_zext(Instruction::OpID op, ConstantInt *value1) {
    int const_value = value1->get_value();
    if (op == Instruction::zext) {
        return ConstantInt::get(const_value, module_);
    }
    return nullptr;
}

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2) {
    int const_value1 = value1->get_value();
    int const_value2 = value2->get_value();
    switch (op) {
    case Instruction::add:
        return ConstantInt::get(const_value1 + const_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(const_value1 - const_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(const_value1 * const_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(const_value1 / const_value2), module_);
        break;
    case Instruction::srem:
        return ConstantInt::get(const_value1 % const_value2, module_);
        break;
    case Instruction::land:
        return ConstantInt::get(const_value1 & const_value2, module_);
        break;
    case Instruction::lor:
        return ConstantInt::get(const_value1 | const_value2, module_);
        break;
    case Instruction::lxor:
        return ConstantInt::get(const_value1 ^ const_value2, module_);
        break;
    case Instruction::asr:
        return ConstantInt::get((int)((signed int)(const_value1) >> const_value2), module_);
        break;
    case Instruction::lsl:
        return ConstantInt::get(const_value1 << const_value2, module_);
        break;
    case Instruction::lsr:
        return ConstantInt::get((int)((unsigned int)(const_value1) >> const_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

ConstantInt *ConstFolder::compute(CmpInst::CmpOp op, ConstantInt *value1, ConstantInt *value2) {
    int const_value1 = value1->get_value();
    int const_value2 = value2->get_value();
    switch (op)
    {
    case CmpInst::EQ:
        return ConstantInt::get(const_value1 == const_value2, module_);
        break;
    case CmpInst::NE:
        return ConstantInt::get(const_value1 != const_value2, module_);
        break;
    case CmpInst::GT:
        return ConstantInt::get(const_value1 > const_value2, module_);
        break;
    case CmpInst::GE:
        return ConstantInt::get(const_value1 >= const_value2, module_);
        break;
    case CmpInst::LT:
        return ConstantInt::get(const_value1 < const_value2, module_);
        break;
    case CmpInst::LE:
        return ConstantInt::get(const_value1 <= const_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

void ConstPropagation::execute() {
    for (auto func : module->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            bb_ = bb;
            const_propagation();
        }
    }
    reduce_redundant_cond_br();
    return ;
}

void ConstPropagation::reduce_redundant_cond_br() {
    for (auto func : module->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            builder->set_insert_point(bb);
            if (bb->get_terminator()->is_br()) {
                auto br = bb->get_terminator();
                if (dynamic_cast<BranchInst *>(br)->is_cond_br()) {
                    auto cond = dynamic_cast<ConstantInt*>(br->get_operand(0));
                    auto trueBB = br->get_operand(1);
                    auto falseBB = br->get_operand(2);
                    BasicBlock *surviveBB = nullptr;
                    if (cond) {
                        if (cond->get_value() == 1) {
                            surviveBB = dynamic_cast<BasicBlock *>(trueBB);
                        }
                        else {
                            surviveBB = dynamic_cast<BasicBlock *>(falseBB);
                        }
                        for (auto succBB : bb->get_succ_basic_blocks()) {
                            if (succBB != surviveBB) {
                                succBB->remove_pre_basic_block(bb);
                                for (auto instr : succBB->get_instructions()) {
                                    if (instr->is_phi()) {
                                        for (int i = 1; i < instr->get_num_operand(); i+=2) {
                                            if (instr->get_operand(i) == bb) {
                                                instr->remove_operands(i - 1, i);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        bb->delete_instr(br);
                        builder->create_br(surviveBB);
                        bb->get_succ_basic_blocks().clear();
                        bb->add_succ_basic_block(surviveBB);
                    }
                }
            }
        }
    }
}

void ConstPropagation::const_propagation() {
    const_global_var.clear();
    std::vector<Instruction *> wait_delete;
    for (auto instr : bb_->get_instructions()) {
        if (instr->isBinary()) {
            auto value1 = dynamic_cast<ConstantInt*>(instr->get_operand(0));
            auto value2 = dynamic_cast<ConstantInt*>(instr->get_operand(1));
            if (value1 && value2) {
                auto folder_const = folder->compute(instr->get_instr_type(), value1, value2);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_cmp()) {
            auto value1 = dynamic_cast<ConstantInt*>(instr->get_operand(0));
            auto value2 = dynamic_cast<ConstantInt*>(instr->get_operand(1));
            if (value1 && value2) {
                auto folder_const = folder->compute(dynamic_cast<CmpInst *>(instr)->get_cmp_op(), value1, value2);
                instr->replace_all_use_with(folder_const); 
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_zext()) {
            auto value1 = dynamic_cast<ConstantInt*>(instr->get_operand(0));
            if (value1) {
                auto folder_const = folder->compute_zext(instr->get_instr_type(), value1);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_load()) {
            auto value1 = get_global_const_val(dynamic_cast<LoadInst *>(instr)->get_lval());
            if (value1) {
                instr->replace_all_use_with(value1);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_store()) {
            auto value1 = dynamic_cast<StoreInst *>(instr)->get_lval();
            auto value2 = dynamic_cast<ConstantInt*>(dynamic_cast<StoreInst *>(instr)->get_rval());
            if (value2) {
                set_global_const_val(value1, value2);
            }
            else {
                if (const_global_var.find(value1) != const_global_var.end()) {
                    const_global_var.erase(const_global_var.find(value1));
                } else {
                    auto ptr_ins = dynamic_cast<Instruction*>(value1);
                    if (ptr_ins != nullptr) {
                        Value *base;
                        ConstantInt *const_offset;
                        if (ptr_ins->is_gep()) {
                            base = ptr_ins->get_operand(0);
                            const_offset = ConstantInt::get(0, module);
                        } else if (ptr_ins->is_add()) {
                            base = dynamic_cast<Instruction*>(ptr_ins->get_operand(0))->get_operand(0);
                            const_offset = dynamic_cast<ConstantInt*>(ptr_ins->get_operand(1));
                        } else {
                            std::cerr<<"bad store variable, incorrect instruction!"<<std::endl;
                            exit(-1);
                        }
                        if (const_offset) {
                            if (const_array[base].find(const_offset->get_value()) != const_array[base].end()) {
                                const_array[base].erase(const_array[base].find(const_offset->get_value()));
                            }
                        }
                    }
                }
            }
        }
    }
    for (auto instr : wait_delete) {
        bb_->delete_instr(instr);
    }
    return ;
}

Constant *ConstPropagation::get_global_const_val(Value *value) {
    auto global_value = dynamic_cast<GlobalVariable *>(value);
    auto ptr_ins = dynamic_cast<Instruction*>(value);
    if (global_value != nullptr) {
        if (const_global_var.find(value) != const_global_var.end()) {
            return const_global_var[value];
        }
    } else if (ptr_ins != nullptr) {
        Value *base;
        ConstantInt *const_offset;
        if (ptr_ins->is_gep()) {
            base = ptr_ins;
            const_offset = ConstantInt::get(0, module);
        } else if (ptr_ins->is_add()) {
            base = dynamic_cast<Instruction*>(ptr_ins->get_operand(0))->get_operand(0);
            const_offset = dynamic_cast<ConstantInt*>(ptr_ins->get_operand(1));
        } else {
            std::cerr<<"bad load, incorrect instruction!"<<std::endl;
            exit(-1);
        }
        if (const_offset) {
            if (const_array.find(base) != const_array.end()) {
                if (const_array[base].find(const_offset->get_value()) != const_array[base].end()) {
                    return const_array[base][const_offset->get_value()];
                }
            }
        }
    }
    return nullptr;
}

Constant *ConstPropagation::set_global_const_val(Value *value, ConstantInt *const_val) {
    auto global_value = dynamic_cast<GlobalVariable *>(value);
    auto ptr_ins = dynamic_cast<Instruction*>(value);
    if (global_value != nullptr) {
        if (const_global_var.find(global_value) != const_global_var.end()) {
            const_global_var[global_value] = const_val;
        } else {
            const_global_var.insert({global_value, const_val});
        }
        return const_val;
    } else if (ptr_ins != nullptr) {
        Value *base;
        ConstantInt *const_offset;
        if (ptr_ins->is_gep()) {
            base = ptr_ins;
            const_offset = ConstantInt::get(0, module);
        } else if (ptr_ins->is_add()) {
            base = dynamic_cast<Instruction*>(ptr_ins->get_operand(0))->get_operand(0);
            const_offset = dynamic_cast<ConstantInt*>(ptr_ins->get_operand(1));
        } else {
            std::cerr<<"bad store const, incorrect instruction!"<<std::endl;
            exit(-1);
        }
        if (const_offset) {
            if (const_array.find(base) != const_array.end()) {
                if (const_array[base].find(const_offset->get_value()) != const_array[base].end()) {
                    const_array[base][const_offset->get_value()] = const_val;
                } else {
                    const_array[base].insert({const_offset->get_value(), const_val});
                }
            } else {
                const_array.insert({base, {}});
                const_array[base].insert({const_offset->get_value(), const_val});
            }
            return const_val;
        }
    }
    return nullptr;
}
