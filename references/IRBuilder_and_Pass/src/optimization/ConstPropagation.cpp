#include "ConstPropagation.hpp"
#include "logging.hpp"

// 给出了返回整型值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式

// 处理零扩展的常数折叠
ConstantInt *ConstFolder::compute_zext(
    Instruction::OpID op,
    ConstantInt *value1)
{
    int c_value1 = value1->get_value();
    if (op == Instruction::zext) {
        return ConstantInt::get(c_value1, module_);
    }
    return nullptr;
}

// 处理整型值四则运算的常数折叠
ConstantInt *ConstFolder::compute(
    Instruction::OpID op,
    ConstantInt *value1,
    ConstantInt *value2)
{
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// 处理浮点值转整型值的常数折叠
ConstantInt *ConstFolder::compute(
    Instruction::OpID op,
    ConstantFP *value1)
{
    float c_value1 = value1->get_value();
    if (op == Instruction::fptosi) {
        return ConstantInt::get((int) c_value1, module_);
    }
    return nullptr;
}

// 处理浮点值四则运算的常数折叠
ConstantFP *ConstFolder::compute(
    Instruction::OpID op,
    ConstantFP *value1,
    ConstantFP *value2)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        return ConstantFP::get(c_value1 / c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// 处理整型值转浮点值的常数折叠
ConstantFP *ConstFolder::compute(
    Instruction::OpID op,
    ConstantInt *value1)
{
    int c_value1 = value1->get_value();
    if (op == Instruction::sitofp) {
        return ConstantFP::get((float) c_value1, module_);
    }
    return nullptr;
}

// 处理整型值比较结果的常数折叠
ConstantInt *ConstFolder::compute(
    CmpInst::CmpOp op,
    ConstantInt *value1,
    ConstantInt *value2)
{

    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case CmpInst::EQ:
        return ConstantInt::get(c_value1 == c_value2, module_);
        break;
    case CmpInst::NE:
        return ConstantInt::get(c_value1 != c_value2, module_);
        break;
    case CmpInst::GT:
        return ConstantInt::get(c_value1 > c_value2, module_);
        break;
    case CmpInst::GE:
        return ConstantInt::get(c_value1 >= c_value2, module_);
        break;
    case CmpInst::LT:
        return ConstantInt::get(c_value1 < c_value2, module_);
        break;
    case CmpInst::LE:
        return ConstantInt::get(c_value1 <= c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// 处理浮点值比较结果的常数折叠
ConstantInt *ConstFolder::compute(
    FCmpInst::CmpOp op,
    ConstantFP *value1,
    ConstantFP *value2)
{

    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
    case FCmpInst::EQ:
        return ConstantInt::get(c_value1 == c_value2, module_);
        break;
    case FCmpInst::NE:
        return ConstantInt::get(c_value1 != c_value2, module_);
        break;
    case FCmpInst::GT:
        return ConstantInt::get(c_value1 > c_value2, module_);
        break;
    case FCmpInst::GE:
        return ConstantInt::get(c_value1 >= c_value2, module_);
        break;
    case FCmpInst::LT:
        return ConstantInt::get(c_value1 < c_value2, module_);
        break;
    case FCmpInst::LE:
        return ConstantInt::get(c_value1 <= c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    else
    {
        return nullptr;
    }
}

ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    else
    {
        return nullptr;
    }
}

void ConstPropagation::run()
{
    // 从这里开始吧！
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            BB = bb;
            const_propagation();
        }
    }
    reduce_redundant_cond_br();
    return ;
}

void ConstPropagation::reduce_redundant_cond_br() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            builder->set_insert_point(bb);
            if (bb->get_terminator()->is_br()) {
                auto br = bb->get_terminator();
                if (dynamic_cast<BranchInst *>(br)->is_cond_br()) {
                    auto cond = cast_constantint(br->get_operand(0));
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
                        // surviveBB->add_pre_basic_block(bb);
                    }
                }
            }
        }
    }
}

void ConstPropagation::const_propagation() {
    const_global_var.clear();
    std::vector<Instruction *> wait_delete;
    for (auto instr : BB->get_instructions()) {
        if (instr->is_add() || instr->is_sub() || instr->is_mul() || instr->is_div()) {
            auto value1 = cast_constantint(instr->get_operand(0));
            auto value2 = cast_constantint(instr->get_operand(1));
            if (value1 && value2) {
                auto folder_const = folder->compute(instr->get_instr_type(), value1, value2);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_fadd() || instr->is_fsub() || instr->is_fmul() || instr->is_fdiv()) {
            auto value1 = cast_constantfp(instr->get_operand(0));
            auto value2 = cast_constantfp(instr->get_operand(1));
            if (value1 && value2) {
                auto folder_const = folder->compute(instr->get_instr_type(), value1, value2);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_cmp()) {
            auto value1 = cast_constantint(instr->get_operand(0));
            auto value2 = cast_constantint(instr->get_operand(1));
            if (value1 && value2) {
                auto folder_const = folder->compute(dynamic_cast<CmpInst *>(instr)->get_cmp_op(), value1, value2);
                instr->replace_all_use_with(folder_const); 
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_fcmp()) {
            auto value1 = cast_constantfp(instr->get_operand(0));
            auto value2 = cast_constantfp(instr->get_operand(1));
            if (value1 && value2) {
                auto folder_const = folder->compute(dynamic_cast<FCmpInst *>(instr)->get_cmp_op(), value1, value2);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_fp2si()) {
            auto value1 = cast_constantfp(instr->get_operand(0));
            if (value1) {
                auto folder_const = folder->compute(instr->get_instr_type(), value1);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_si2fp()) {
            auto value1 = cast_constantint(instr->get_operand(0));
            if (value1) {
                auto folder_const = folder->compute(instr->get_instr_type(), value1);
                instr->replace_all_use_with(folder_const);
                wait_delete.push_back(instr);
            }
        }
        else if (instr->is_zext()) {
            auto value1 = cast_constantint(instr->get_operand(0));
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
            auto value2 = cast_constantint(dynamic_cast<StoreInst *>(instr)->get_rval());
            auto value3 = cast_constantfp(dynamic_cast<StoreInst *>(instr)->get_rval());
            if (value2) {
                set_global_const_val(value1, value2);
            }
            else if (value3) {
                set_global_const_val(value1, value3);
            }
            else {
                if (const_global_var.find(value1) != const_global_var.end()) {
                    const_global_var.erase(const_global_var.find(value1));
                }
            }
        }
    }
    for (auto instr : wait_delete) {
        BB->delete_instr(instr);
    }
    return ;
}

// 可同时处理整型值和浮点值的全局变量
Constant *ConstPropagation::get_global_const_val(Value *value) {
    auto global_value = dynamic_cast<GlobalVariable *>(value);
    if (global_value != nullptr) {
        if (const_global_var.find(value) != const_global_var.end()) {
            return const_global_var.find(value)->second;
        }
    }
    return nullptr;
}

// 可同时处理整型值和浮点值的全局变量
Constant *ConstPropagation::set_global_const_val(Value *value, Constant *const_val) {
    auto global_value = dynamic_cast<GlobalVariable *>(value);
    if (global_value != nullptr) {
        if (const_global_var.find(global_value) != const_global_var.end()) {
            const_global_var.find(global_value)->second = const_val;
            return const_val;
        }
        else {
            const_global_var.insert({global_value, const_val});
            return const_val;
        }
    }
    return nullptr;
}
