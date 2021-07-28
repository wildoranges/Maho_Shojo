#ifndef MHSJ_CONSTPROPAGATION_H
#define MHSJ_CONSTPROPAGATION_H
#include "Pass.h"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"
#include "Value.h"
#include "IRBuilder.h"

class ConstFolder
{
public:
    ConstFolder(Module *module) : module_(module) {}
    ConstantInt *compute_zext(Instruction::OpID op, ConstantInt *value1);
    ConstantInt *compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2);
    ConstantInt *compute(CmpInst::CmpOp op, ConstantInt *value1, ConstantInt *value2);
private:
    Module *module_;
};

class ConstPropagation : public Pass
{
private:
    BasicBlock *bb_;
    IRBuilder *builder;
    ConstFolder * folder;
    std::map<Value*, ConstantInt*> const_global_var;
    std::map<Value*, std::map<unsigned int, ConstantInt*>> const_array;

public:
    ConstPropagation(Module *module) : Pass(module) {
        folder = new ConstFolder(module);
        builder = new IRBuilder(nullptr, module);
    }
    ~ConstPropagation() { delete folder; }

    void execute() final;
    void const_propagation();
    void reduce_redundant_cond_br();

    Constant *get_global_const_val(Value *value);
    Constant *set_global_const_val(Value *value, ConstantInt *const_val);
};

#endif  // MHSJ_CONSTPROPAGATION_H