#include "cminusf_builder.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_ZERO(var_type) \
    ConstantZero::get(var_type, module.get())
#define CONST_INT(num) \
    ConstantInt::get((int)num, module.get())
#define Int32Type \
    Type::get_int32_type(module.get())
#define FloatType \
    Type::get_float_type(module.get())
#define VoidType \
    Type::get_void_type(module.get())
#define Int32PtrType \
    Type::get_int32_ptr_type(module.get())
#define FloatPtrType \
    Type::get_float_ptr_type(module.get())

// You can define global variables here
// to store state

std::vector<std::shared_ptr<ASTParam>> Params;
std::vector<Type*> ParamsType;
Value *Address; //等式左边的地址
Value *ExpressionValue;//等式右边的值
AllocaInst * retAlloca;
BasicBlock *retBB;
bool is_left_var_assign = false;
bool is_var_assign = false;
bool have_return = false;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    for (auto decl: node.declarations) {
        decl.get()->accept(*this);
    }
    CminusfBuilder::scope.exit();
}

void CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT) {
        ExpressionValue = CONST_INT(node.i_val);
    }
    else if (node.type == TYPE_FLOAT) {
        ExpressionValue = CONST_FP(node.f_val);
    }
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *varTy;
    if (node.type == TYPE_INT) {
        varTy = Int32Type;
    }
    else if (node.type == TYPE_FLOAT) {
        varTy = FloatType;
    }
    if (node.num != nullptr) {
        node.num.get()->accept(*this);
        auto IntValue = node.num.get()->i_val;
        varTy = ArrayType::get(varTy, IntValue);
    }
    if (CminusfBuilder::scope.in_global()) {
        CminusfBuilder::scope.push(node.id, GlobalVariable::create(node.id, module.get(), varTy, false, CONST_ZERO(varTy)));
    }
    else {
        CminusfBuilder::scope.push(node.id, builder->create_alloca(varTy));
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    Type *FuncType;
    if (node.type == TYPE_VOID) {
        FuncType = VoidType;
    }
    else if (node.type == TYPE_INT) {
        FuncType = Int32Type;
    }
    else if (node.type == TYPE_FLOAT) {
        FuncType = FloatType;
    }
    for (auto param: node.params) {
        param.get()->accept(*this);
    }
    Params = node.params;
    auto calleeTy = FunctionType::get(FuncType, ParamsType);
    auto fun_ptr = Function::create(calleeTy, node.id, module.get());
    CminusfBuilder::scope.push(node.id, fun_ptr);
    std::vector<Type*>().swap(ParamsType);
    auto bb = BasicBlock::create(module.get(), "", fun_ptr);
    builder->set_insert_point(bb);
    retBB = BasicBlock::create(module.get(), "", fun_ptr);
    if (FuncType != VoidType) {
        retAlloca = builder->create_alloca(FuncType);
    }
    node.compound_stmt.get()->accept(*this);
    builder->create_br(retBB);
    builder->set_insert_point(retBB);
    if (FuncType == VoidType) {
        builder->create_void_ret();
    }
    else {
        builder->create_ret(builder->create_load(retAlloca));
    }
}

void CminusfBuilder::visit(ASTParam &node) {
    if (node.id.std::string::compare("void") != 0) {
        Type *ParamType;
        if (node.isarray) {
            if (node.type == TYPE_INT) {
                ParamType = Int32PtrType;
            }
            else if (node.type == TYPE_FLOAT) {
                ParamType = FloatPtrType;
            }
        }
        else {
            if (node.type == TYPE_INT) {
                ParamType = Int32Type;
            }
            else if (node.type == TYPE_FLOAT) {
                ParamType = FloatType;
            }
        }        
        ParamsType.push_back(ParamType);
    }
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    CminusfBuilder::scope.enter();
    auto fun_ptr = builder->get_insert_block()->get_parent();
    std::vector<Value *> args;
    for (auto arg = fun_ptr->arg_begin(); arg != fun_ptr->arg_end(); arg++) {
        args.push_back(*arg);
    }
    auto param = Params.begin();
    for (auto arg: args) {
        Type *argTy;
        auto argGetType = arg->get_type();
        if (argGetType->is_integer_type()) {
            argTy = Int32Type;
        }
        else if (argGetType->is_float_type()) {
            argTy = FloatType;
        }
        else if (argGetType->is_pointer_type()) {
            if (argGetType->get_pointer_element_type()->is_integer_type()) {
                argTy = Int32PtrType;
            }
            else if (argGetType->get_pointer_element_type()->is_float_type()) {
                argTy = FloatPtrType;
            }
        }
        auto argAlloca = builder->create_alloca(argTy);
        builder->create_store(arg, argAlloca);
        CminusfBuilder::scope.push((*param++).get()->id, argAlloca);
    }
    for (auto decl: node.local_declarations) {
        decl.get()->accept(*this);
    }
    for (auto stmt: node.statement_list) {
        stmt.get()->accept(*this);
    }
    CminusfBuilder::scope.exit();
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    if (node.expression != nullptr) {
        node.expression.get()->accept(*this);
    }
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    auto fun_ptr = builder->get_insert_block()->get_parent();
    auto trueBB = BasicBlock::create(module.get(), "", fun_ptr);
    auto falseBB = BasicBlock::create(module.get(), "", fun_ptr);
    auto nextBB = BasicBlock::create(module.get(), "", fun_ptr);
    node.expression.get()->accept(*this);
    auto ExpressionValueType = ExpressionValue->get_type();
    if (ExpressionValueType->is_integer_type()) {
        auto icmp = builder->create_icmp_ne(ExpressionValue, CONST_INT(0));
        builder->create_cond_br(icmp, trueBB, falseBB);
    }
    else if (ExpressionValueType->is_float_type()) {
        auto fcmp = builder->create_fcmp_ne(ExpressionValue, CONST_FP(0.0));
        builder->create_cond_br(fcmp, trueBB, falseBB);
    }
    builder->set_insert_point(trueBB);
    have_return = false;
    CminusfBuilder::scope.enter();
    node.if_statement.get()->accept(*this);
    CminusfBuilder::scope.exit();
    if (have_return == false) {
        builder->create_br(nextBB);
    }
    else {
        builder->create_br(retBB);
    }
    have_return = false;
    builder->set_insert_point(falseBB);
    if (node.else_statement != nullptr) {
        CminusfBuilder::scope.enter();
        node.else_statement.get()->accept(*this);
        CminusfBuilder::scope.exit();
    }
    if (have_return == false) {
        builder->create_br(nextBB);
    }
    else {
        builder->create_br(retBB);
    }
    builder->set_insert_point(nextBB);
    have_return = false;
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    Value *cmp;
    auto fun_ptr = builder->get_insert_block()->get_parent();
    auto trueBB = BasicBlock::create(module.get(), "", fun_ptr);
    auto nextBB = BasicBlock::create(module.get(), "", fun_ptr);
    auto whileBB = BasicBlock::create(module.get(), "", fun_ptr);
    builder->create_br(whileBB);
    builder->set_insert_point(whileBB);
    node.expression.get()->accept(*this);
    auto ExpressionValueType = ExpressionValue->get_type();
    if (ExpressionValueType->is_integer_type()) {
        cmp = builder->create_icmp_ne(ExpressionValue, CONST_INT(0));
    }
    else if (ExpressionValueType->is_float_type()) {
        cmp = builder->create_fcmp_ne(ExpressionValue, CONST_FP(0.0));
    }
    builder->create_cond_br(cmp, trueBB, nextBB);
    builder->set_insert_point(trueBB);
    have_return = false;
    CminusfBuilder::scope.enter();
    node.statement.get()->accept(*this);
    CminusfBuilder::scope.exit();
    if (have_return == false) {
        builder->create_br(whileBB);
    }
    else {
        builder->create_br(retBB);
    }
    builder->set_insert_point(nextBB);
    have_return = false;
}

void CminusfBuilder::visit(ASTReturnStmt &node) {
    auto fun_ptr = builder->get_insert_block()->get_parent();
    auto returnTy = fun_ptr->get_return_type();
    if (node.expression == nullptr) {
    }
    else {
        node.expression.get()->accept(*this);
        auto ExpressionValueType = ExpressionValue->get_type();
        if (ExpressionValueType != returnTy) {
            if(returnTy->is_float_type()){
                auto tmpi2f = builder->create_sitofp(ExpressionValue,FloatType);
                ExpressionValue = tmpi2f;
            }
            else if(returnTy->is_integer_type()){
                    auto tmpf2i = builder->create_fptosi(ExpressionValue,Int32Type);
                    ExpressionValue = tmpf2i;
                }
        }
        builder->create_store(ExpressionValue, retAlloca);
    }
    have_return = true;
}

void CminusfBuilder::visit(ASTVar &node) {
    auto fun_ptr = builder->get_insert_block()->get_parent();
    Value* tmpAddress; 
    if (node.expression != nullptr) {
        auto errorBB = BasicBlock::create(module.get(), "", fun_ptr);
        auto nextBB = BasicBlock::create(module.get(), "", fun_ptr);
        auto tmp_is_left_var_assign = is_left_var_assign;
        is_left_var_assign = false;
        node.expression.get()->accept(*this);
        is_left_var_assign = tmp_is_left_var_assign;
        auto ExpressionValueType = ExpressionValue->get_type();
        if(ExpressionValueType->is_float_type()){
            auto tmpf2i = builder->create_fptosi(ExpressionValue,Int32Type);
            ExpressionValue = tmpf2i;
        }
        auto icmp = builder->create_icmp_lt(ExpressionValue, CONST_INT(0));
        builder->create_cond_br(icmp, errorBB, nextBB);
        builder->set_insert_point(errorBB);
        builder->create_call(CminusfBuilder::scope.find("neg_idx_except"), {});
        builder->create_br(retBB);
        builder->set_insert_point(nextBB);
        tmpAddress = CminusfBuilder::scope.find(node.id);
        tmpAddress = builder->create_gep(tmpAddress, {CONST_INT(0), ExpressionValue});//get the add of ID[EXP]
        if(is_left_var_assign == true && is_var_assign ==true){
            Address = tmpAddress;
        }
    }
    else {
        tmpAddress = CminusfBuilder::scope.find(node.id);
        if(is_left_var_assign == true && is_var_assign ==true){
            Address = tmpAddress;
        }
    }
    if ((is_left_var_assign == false && is_var_assign == true) || is_var_assign == false) {
        ExpressionValue = builder->create_load(tmpAddress);
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    is_var_assign = true;
    is_left_var_assign = true;
    node.var.get()->accept(*this);
    auto varTy = Address->get_type()->get_pointer_element_type();
    is_left_var_assign = false;
    node.expression.get()->accept(*this);
    auto ExpressionValueTy = ExpressionValue->get_type();
    if(varTy->is_float_type()){
        if(ExpressionValueTy->is_integer_type()){
            auto tmpi2f = builder->create_sitofp(ExpressionValue,FloatType);
            ExpressionValue = tmpi2f;
        }
    }
    if(varTy->is_integer_type()){
        if(ExpressionValueTy->is_float_type()){
            auto tmpf2i = builder->create_fptosi(ExpressionValue,Int32Type);
            ExpressionValue = tmpf2i;
        }
    }
    builder->create_store(ExpressionValue, Address);
    is_var_assign = false;
    is_left_var_assign = false;
}

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    Value *cmp;
    node.additive_expression_l.get()->accept(*this);
    if (node.additive_expression_r == nullptr) {
        return ;
    }
    else {
        bool is_float = false;
        auto tmpterm = ExpressionValue;
        auto tmptermTy = tmpterm->get_type();
        node.additive_expression_r.get()->accept(*this);
        auto ExpressionValueType = ExpressionValue->get_type();
        if (tmptermTy->is_float_type()) {
            is_float = true;
            if (ExpressionValueType->is_integer_type()) {
                auto tmpi2f = builder->create_sitofp(ExpressionValue,FloatType);
                ExpressionValue = tmpi2f;
            }
        }
        else {
            if (ExpressionValueType->is_float_type()) {
                is_float = true;
                auto tmpi2f = builder->create_sitofp(tmpterm,FloatType);
                tmpterm = tmpi2f;
            }
        }
        if (node.op == OP_LT) {
            if (is_float) {
                cmp = builder->create_fcmp_lt(tmpterm, ExpressionValue);
            }
            else {
                cmp = builder->create_icmp_lt(tmpterm, ExpressionValue);
            }
        }
        else if (node.op == OP_LE) {
            if (is_float) {
                cmp = builder->create_fcmp_le(tmpterm, ExpressionValue);
            }
            else {
                cmp = builder->create_icmp_le(tmpterm, ExpressionValue);
            }
        }
        else if (node.op == OP_GE) {
            if (is_float) {
                cmp = builder->create_fcmp_ge(tmpterm, ExpressionValue);
            }
            else {
                cmp = builder->create_icmp_ge(tmpterm, ExpressionValue);
            }
        }
        else if (node.op == OP_GT) {
            if (is_float) {
                cmp = builder->create_fcmp_gt(tmpterm, ExpressionValue);
            }
            else {
                cmp = builder->create_icmp_gt(tmpterm, ExpressionValue);
            }
        }
        else if (node.op == OP_EQ) {
            if (is_float) {
                cmp = builder->create_fcmp_eq(tmpterm, ExpressionValue);
            }
            else {
                cmp = builder->create_icmp_eq(tmpterm, ExpressionValue);
            }
        }
        else if (node.op == OP_NEQ) {
            if (is_float) {
                cmp = builder->create_fcmp_ne(tmpterm, ExpressionValue);
            }
            else {
                cmp = builder->create_icmp_ne(tmpterm, ExpressionValue);
            }
        }
        else {
            std::abort();
        }
        ExpressionValue = builder->create_zext(cmp, Int32Type);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if (node.additive_expression == nullptr) {
        node.term.get()->accept(*this);
        return ;
    }
    else {
        bool is_float = false;
        node.additive_expression.get()->accept(*this);
        auto tmpadd_exp = ExpressionValue;
        auto tmpadd_expType = tmpadd_exp->get_type();
        node.term.get()->accept(*this);
        auto tmpterm = ExpressionValue;
        auto tmptermType = tmpterm->get_type();
        if (tmpadd_expType->is_float_type()) {
            is_float = true;
            if (tmptermType->is_integer_type()) {
                auto tmpi2f = builder->create_sitofp(tmpterm,FloatType);
                tmpterm = tmpi2f;
            }
        }
        else {
            if (tmptermType->is_float_type()) {
                is_float = true;
                auto tmpi2f = builder->create_sitofp(tmpadd_exp,FloatType);
                tmpadd_exp = tmpi2f;
            }
        }
        if (node.op == OP_PLUS) {
            if (is_float) {
                ExpressionValue = builder->create_fadd(tmpadd_exp, tmpterm);
            }
            else {
                ExpressionValue = builder->create_iadd(tmpadd_exp, tmpterm);
            }
        }
        else if (node.op == OP_MINUS) {
            if (is_float) {
                ExpressionValue = builder->create_fsub(tmpadd_exp, tmpterm);
            }
            else {
                ExpressionValue = builder->create_isub(tmpadd_exp, tmpterm);
            }
        }
        else {
            std::abort();
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    if (node.term == nullptr) {
        node.factor.get()->accept(*this);
        return ;
    }
    else {
        bool is_float = false;
        node.term.get()->accept(*this);
        auto tmpterm = ExpressionValue;
        auto tmptermTy = tmpterm->get_type();
        node.factor.get()->accept(*this);
        auto tmpfactor = ExpressionValue;
        auto tmpfactorTy = tmpfactor->get_type();
        if (tmptermTy->is_float_type()) {
            is_float = true;
            if (tmpfactorTy->is_integer_type()) {
                auto tmpi2f = builder->create_sitofp(tmpfactor,FloatType);
                tmpfactor = tmpi2f;
            }
        }
        else {
            if (tmpfactorTy->is_float_type()) {
                is_float = true;
                auto tmpi2f = builder->create_sitofp(tmpterm,FloatType);
                tmpterm = tmpi2f;
            }
        }
        if (node.op == OP_MUL) {
            if (is_float) {
                ExpressionValue = builder->create_fmul(tmpterm, tmpfactor);
            }
            else {
                ExpressionValue = builder->create_imul(tmpterm, tmpfactor);
            }
        }
        else if (node.op == OP_DIV) {
            if (is_float) {
                ExpressionValue = builder->create_fdiv(tmpterm, tmpfactor);
            }
            else {
                ExpressionValue = builder->create_isdiv(tmpterm, tmpfactor);
            }
        }
        else {
            std::abort();
        }
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    FunctionType * calleeTy;
    unsigned int i = 0;
    auto fun_ptr = CminusfBuilder::scope.find(node.id);
    std::vector<Value*> Arguments;
    auto funTy = fun_ptr->get_type();
    if(fun_ptr->get_type()->is_function_type()) {
        calleeTy = static_cast <FunctionType*> (funTy);
        for (auto arg: node.args) {
            arg.get()->accept(*this);
            auto ExpressionValueType = ExpressionValue->get_type();
            auto paramTy = calleeTy->get_param_type(i++);
            if (paramTy->is_float_type() && ExpressionValueType->is_integer_type()) {
                auto tmpi2f = builder->create_sitofp(ExpressionValue,FloatType);
                Arguments.push_back(tmpi2f);

            }
            else if (paramTy->is_integer_type() && ExpressionValueType->is_float_type()) {
                auto tmpf2i = builder->create_fptosi(ExpressionValue,Int32Type);
                Arguments.push_back(tmpf2i);
                }else{
                    Arguments.push_back(ExpressionValue);
                }
        }
        ExpressionValue = builder->create_call(fun_ptr, Arguments);
        have_return = false;
        std::vector<Value*>().swap(Arguments);
    }
    else {
        std::abort();
    }
}
