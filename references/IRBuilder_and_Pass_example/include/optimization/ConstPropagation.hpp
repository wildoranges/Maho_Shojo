#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>

// tips: 用来判断value是否为ConstantFP/ConstantInt
ConstantFP* cast_constantfp(Value *value);
ConstantInt* cast_constantint(Value *value);


// tips: ConstFloder类

class ConstFolder
{
public:
    ConstFolder(Module *m) : module_(m) {}
    ConstantInt *compute_zext(
        Instruction::OpID op,
        ConstantInt *value1);
    ConstantInt *compute(
        Instruction::OpID op,
        ConstantInt *value1,
        ConstantInt *value2);
    ConstantInt *compute(
        Instruction::OpID op,
        ConstantFP *value1);
    ConstantFP *compute(
        Instruction::OpID op,
        ConstantFP *value1,
        ConstantFP *value2);
    ConstantFP *compute(
        Instruction::OpID op,
        ConstantInt *value1);
    ConstantInt *compute(
        CmpInst::CmpOp op,
        ConstantInt *value1,
        ConstantInt *value2);
    ConstantInt *compute(
        FCmpInst::CmpOp op,
        ConstantFP *value1,
        ConstantFP *value2);
    // ...
private:
    Module *module_;
};

class ConstPropagation : public Pass
{
private:
    BasicBlock *BB;
    IRBuilder *builder;
    ConstFolder * folder;
    std::map<Value *, Constant *> const_global_var;

public:
    ConstPropagation(Module *m) : Pass(m) {
        folder = new ConstFolder(m);
        builder = new IRBuilder(nullptr, m);
    }
    ~ConstPropagation() { delete folder; }

    void run();
    void const_propagation();
    void reduce_redundant_cond_br();

    Constant *get_global_const_val(Value *value);
    Constant *set_global_const_val(Value *value, Constant *const_val);
};

#endif