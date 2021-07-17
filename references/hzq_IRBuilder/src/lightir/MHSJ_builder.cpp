/*
 * 声明：本代码为 2020 秋 中国科大编译原理（李诚）课程实验参考实现。
 * 请不要以任何方式，将本代码上传到可以公开访问的站点或仓库
*/

#include "MHSJ_builder.hpp"



#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())

// You can define global variables here
// to store state

// store temporary value
Value *tmp_val = nullptr;
// whether require lvalue
bool require_lvalue = false;
// function that is being built
Function *cur_fun = nullptr;
// detect scope pre-enter (for elegance only)
bool pre_enter_scope = false;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

bool promote(IRBuilder *builder, Value **l_val_p, Value **r_val_p) {
    bool is_int;
    auto &l_val = *l_val_p;
    auto &r_val = *r_val_p;
    if (l_val->get_type() == r_val->get_type()) {
        is_int = l_val->get_type()->is_integer_type();
    } else {
        is_int = false;
        if (l_val->get_type()->is_integer_type())
            l_val = builder->create_sitofp(l_val, FLOAT_T);
        else
            r_val = builder->create_sitofp(r_val, FLOAT_T);
    }
    return is_int;
}


/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());

    for (auto decl: node.declarations) {
        decl->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT)
        tmp_val = CONST_INT(node.i_val);
    else
        tmp_val = CONST_FP(node.f_val);
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *var_type;
    if (node.type == TYPE_INT)
        var_type = Type::get_int32_type(module.get());
    else
        var_type = Type::get_float_type(module.get());
    if (node.num == nullptr) {
        if (scope.in_global()) {
            auto initializer = ConstantZero::get(var_type, module.get());
            auto var = GlobalVariable::create
                    (
                    node.id,
                    module.get(),
                    var_type,
                    false,
                    initializer);
            scope.push(node.id, var);
        } else {
            auto var = builder->create_alloca(var_type);
            scope.push(node.id, var);
        }
    } else {
        auto *array_type = ArrayType::get(var_type, node.num->i_val);
        if (scope.in_global()) {
            auto initializer = ConstantZero::get(array_type, module.get());
            auto var = GlobalVariable::create
                    (
                    node.id,
                    module.get(),
                    array_type,
                    false,
                    initializer);
            scope.push(node.id, var);
        } else {
            auto var = builder->create_alloca(array_type);
            scope.push(node.id, var);
        }
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto& param: node.params) {
        if (param->type == TYPE_INT) {
            if (param->isarray) {
                param_types.push_back(INT32PTR_T);
            } else {
                param_types.push_back(INT32_T);
            }
        } else {
            if (param->isarray) {
                param_types.push_back(FLOATPTR_T);
            } else {
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto fun =
        Function::create(
                fun_type,
                node.id,
                module.get());
    scope.push(node.id, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value*> args;
    for (auto arg = fun->arg_begin();arg != fun->arg_end();arg++) {
        args.push_back(*arg);
    }
    for (int i = 0;i < node.params.size();++i) {
        if (node.params[i]->isarray) {
            Value *array_alloc;
            if (node.params[i]->type == TYPE_INT)
                array_alloc = builder->create_alloca(INT32PTR_T);
            else
                array_alloc = builder->create_alloca(FLOATPTR_T);
            builder->create_store(args[i], array_alloc);
            scope.push(node.params[i]->id, array_alloc);
        } else {
            Value *alloc;
            if (node.params[i]->type == TYPE_INT)
                alloc = builder->create_alloca(INT32_T);
            else
                alloc = builder->create_alloca(FLOAT_T);
            builder->create_store(args[i], alloc);
            scope.push(node.params[i]->id, alloc);
        }
    }
    node.compound_stmt->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr){
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) { }

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    bool need_exit_scope = !pre_enter_scope;
    if (pre_enter_scope) {
        pre_enter_scope = false;
    } else {
        scope.enter();
    }

    for (auto& decl: node.local_declarations) {
        decl->accept(*this);
    }

    for (auto& stmt: node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }

    if (need_exit_scope) {
        scope.exit();
    }
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    if (node.expression != nullptr)
        node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    Value *cond_val;
    if (ret_val->get_type()->is_integer_type())
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    if (node.else_statement == nullptr) {
        builder->create_cond_br(cond_val, trueBB, contBB);
    } else {
        builder->create_cond_br(cond_val, trueBB, falseBB);
    }
    builder->set_insert_point(trueBB);
    node.if_statement->accept(*this);

    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(contBB);

    if (node.else_statement == nullptr) {
        falseBB->erase_from_parent();
    } else {
        builder->set_insert_point(falseBB);
        node.else_statement->accept(*this);
        if (builder->get_insert_block()->get_terminator() == nullptr)
            builder->create_br(contBB);
    }

    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    auto exprBB = BasicBlock::create(module.get(), "", cur_fun);
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(exprBB);
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    Value *cond_val;
    if (ret_val->get_type()->is_integer_type())
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    builder->create_cond_br(cond_val, trueBB, contBB);
    builder->set_insert_point(trueBB);
    node.statement->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
    } else {
        auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
        node.expression->accept(*this);
        if (fun_ret_type != tmp_val->get_type()) {
            if (fun_ret_type->is_integer_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            else
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
        }

        builder->create_ret(tmp_val);
    }
}

void CminusfBuilder::visit(ASTVar &node) {
    auto var = scope.find(node.id);
    assert(var != nullptr);
    auto is_int = var->get_type()->get_pointer_element_type()->is_integer_type();
    auto is_float = var->get_type()->get_pointer_element_type()->is_float_type();
    auto is_ptr = var->get_type()->get_pointer_element_type()->is_pointer_type();
    bool should_return_lvalue = require_lvalue;
    require_lvalue = false;
    if (node.expression == nullptr) {
        if (should_return_lvalue) {
            tmp_val = var;
            require_lvalue = false;
        } else {
            if (is_int || is_float || is_ptr) {
                tmp_val = builder->create_load(var);
            } else {
                tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
            }
        }
    } else {
        node.expression->accept(*this);
        auto val = tmp_val;
        Value *is_neg;
        auto exceptBB = BasicBlock::create(module.get(), "", cur_fun);
        auto contBB = BasicBlock::create(module.get(), "", cur_fun);
        if (val->get_type()->is_float_type())
            val = builder->create_fptosi(val, INT32_T);

        is_neg = builder->create_icmp_lt(val, CONST_INT(0));

        builder->create_cond_br(is_neg, exceptBB, contBB);
        builder->set_insert_point(exceptBB);
        auto neg_idx_except_fun = scope.find("neg_idx_except");
        builder->create_call( static_cast<Function *>(neg_idx_except_fun), {});
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));

        builder->set_insert_point(contBB);
        Value *tmp_ptr;
        if (is_int || is_float)
            tmp_ptr = builder->create_gep(var, {val} );
        else if (is_ptr) {
            auto array_load = builder->create_load(var);
            tmp_ptr = builder->create_gep(array_load, {val} );
        } else
            tmp_ptr = builder->create_gep(var, {CONST_INT(0), val});
        if (should_return_lvalue) {
            tmp_val = tmp_ptr;
            require_lvalue = false;
        } else {
            tmp_val = builder->create_load(tmp_ptr);
        }
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    node.expression->accept(*this);
    auto expr_result = tmp_val;
    require_lvalue = true;
    node.var->accept(*this);
    auto var_addr = tmp_val;
    if (var_addr->get_type()->get_pointer_element_type() != expr_result->get_type()) {
        if (expr_result->get_type() == INT32_T)
            expr_result = builder->create_sitofp(expr_result, FLOAT_T);
        else
            expr_result = builder->create_fptosi(expr_result, INT32_T);
    }
    builder->create_store(expr_result, var_addr);
    tmp_val = expr_result;
}

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    if (node.additive_expression_r == nullptr) {
        node.additive_expression_l->accept(*this);
    } else {
        node.additive_expression_l->accept(*this);
        auto l_val = tmp_val;
        node.additive_expression_r->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        Value *cmp;
        switch (node.op) {
        case OP_LT:
            if (is_int)
                cmp = builder->create_icmp_lt(l_val, r_val);
            else
                cmp = builder->create_fcmp_lt(l_val, r_val);
            break;
        case OP_LE:
            if (is_int)
                cmp = builder->create_icmp_le(l_val, r_val);
            else
                cmp = builder->create_fcmp_le(l_val, r_val);
            break;
        case OP_GE:
            if (is_int)
                cmp = builder->create_icmp_ge(l_val, r_val);
            else
                cmp = builder->create_fcmp_ge(l_val, r_val);
            break;
        case OP_GT:
            if (is_int)
                cmp = builder->create_icmp_gt(l_val, r_val);
            else
                cmp = builder->create_fcmp_gt(l_val, r_val);
            break;
        case OP_EQ:
            if (is_int)
                cmp = builder->create_icmp_eq(l_val, r_val);
            else
                cmp = builder->create_fcmp_eq(l_val, r_val);
            break;
        case OP_NEQ:
            if (is_int)
                cmp = builder->create_icmp_ne(l_val, r_val);
            else
                cmp = builder->create_fcmp_ne(l_val, r_val);
            break;
        }

        tmp_val = builder->create_zext(cmp, INT32_T);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if (node.additive_expression == nullptr) {
        node.term->accept(*this);
    } else {
        node.additive_expression->accept(*this);
        auto l_val = tmp_val;
        node.term->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op) {
        case OP_PLUS:
            if (is_int)
                tmp_val = builder->create_iadd(l_val, r_val);
            else
                tmp_val = builder->create_fadd(l_val, r_val);
            break;
        case OP_MINUS:
            if (is_int)
                tmp_val = builder->create_isub(l_val, r_val);
            else
                tmp_val = builder->create_fsub(l_val, r_val);
            break;
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    if (node.term == nullptr) {
        node.factor->accept(*this);
    } else {
        node.term->accept(*this);
        auto l_val = tmp_val;
        node.factor->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op) {
        case OP_MUL:
            if (is_int)
                tmp_val = builder->create_imul(l_val, r_val);
            else
                tmp_val = builder->create_fmul(l_val, r_val);
            break;
        case OP_DIV:
            if (is_int)
                tmp_val = builder->create_isdiv(l_val, r_val);
            else
                tmp_val = builder->create_fdiv(l_val, r_val);
            break;
        }
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    auto fun = static_cast<Function *>(scope.find(node.id));
    std::vector<Value *> args;
    auto param_type = fun->get_function_type()->param_begin();
    for (auto &arg: node.args) {
        arg->accept(*this);
        if (!tmp_val->get_type()->is_pointer_type() &&
            *param_type != tmp_val->get_type()) {
            if (tmp_val->get_type()->is_integer_type())
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            else
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
        }
        args.push_back(tmp_val);
        param_type++;
    }

    tmp_val = builder->create_call(static_cast<Function *>(fun), args);
}



/*
 * our implement
 */

/* Global Variable */

// used for backpatching
struct true_false_BB
{
    BasicBlock * trueBB = nullptr;
    BasicBlock * falseBB = nullptr;
};

std::list<true_false_BB> IF_While_Stack;        // used for Cond
std::list<true_false_BB> While_Stack;           // used for break and continue
// used for backpatching
std::vector<SyntaxTree::FuncParam>func_fparams;
std::vector<int>array_bounds;
std::vector<int>array_sizes;
int cur_pos;
int cur_depth;
std::map<int, Value*> initval;
/* Global Variable */

void MHSJbuilder::visit(SyntaxTree::Assembly &node){
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    for (auto def: node.global_defs) {
        def->accept(*this);
    }
}

void MHSJbuilder::visit(SyntaxTree::InitVal &node){
    if (node.isExp){
        node.expr->accept(*this);
        initval[cur_pos] = tmp_val;
        cur_pos ++;
    }
    else{
        for (auto elem: node.elementList){
            if (cur_pos % array_sizes[cur_depth-1]){
                cur_pos = (cur_pos/array_sizes[cur_depth-1]+1)*array_sizes[cur_depth-1];
            }
            cur_depth ++;
            elem->accept(*this);
            cur_depth --;
            if (cur_pos % array_sizes[cur_depth-1]){
                cur_pos = (cur_pos/array_sizes[cur_depth-1]+1)*array_sizes[cur_depth-1];
            }
        }
    }
}

void MHSJbuilder::visit(SyntaxTree::FuncDef &node){
    FunctionType *fun_type;
    Type *ret_type;
    if (node.ret_type == SyntaxTree::Type::INT)
        ret_type = INT32_T;
    else
        ret_type = VOID_T;

    std::vector<Type *> param_types;
    node.param_list->accept(*this);
    for (auto param: func_fparams){
        if (param.param_type == SyntaxTree::Type::INT){
            if (param.array_index.empty()){
                param_types.push_back(INT32_T);
            }
            else{
                param_types.push_back(INT32PTR_T);
            }
        }
    }
    fun_type = FunctionType::get(ret_type, param_types);
    auto fun = Function::create(fun_type,node.name,module.get());
    scope.push(node.name, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value*> args;
    for (auto arg = fun->arg_begin();arg != fun->arg_end();arg++) {
        args.push_back(*arg);
    }
    for (int i = 0;i < func_fparams.size(); i++){
        if (func_fparams[i].array_index.empty()){
            Value *alloc;
            alloc = builder->create_alloca(INT32_T);
            builder->create_store(args[i], alloc);
            scope.push(func_fparams[i].name,alloc);
        }
        else{
            Value *alloc_array;
            alloc_array = builder->create_alloca(INT32PTR_T);
            builder->create_store(args[i], alloc_array);
            scope.push(func_fparams[i].name,alloc_array);
        }
    }
    node.body->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr){
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
}

void MHSJbuilder::visit(SyntaxTree::FuncFParamList &node){
    for (auto Param: node.params){
        Param->accept(*this);
    }
}

void MHSJbuilder::visit(SyntaxTree::FuncParam &node){
    func_fparams.push_back(node);
}

void MHSJbuilder::visit(SyntaxTree::VarDef &node){
    Type *var_type;
    if (node.is_constant && 0){
        //TODO
    }
    else{
        var_type = INT32_T;
        if (node.array_length.empty()){
            Value *var;
            if (scope.in_global()) {
                auto initializer = ConstantZero::get(var_type, module.get());
                var = GlobalVariable::create(node.name,module.get(),var_type,false,initializer);
            }
            else {
                var = builder->create_alloca(var_type);
            }
            scope.push(node.name, var);
            if (node.is_inited){
                node.initializers->accept(*this);
                builder->create_store(tmp_val, var);
            }
        }
        else{
            //array
            array_bounds.clear();
            array_sizes.clear();
            for (auto bound_expr: node.array_length){
                bound_expr->accept(*this);
                auto bound_const = dynamic_cast<ConstantInt *>(tmp_val);
                auto bound = bound_const->get_value();
                array_bounds.push_back(bound);
            }
            int total_size = 1;
            for (auto iter = array_bounds.rbegin(); iter != array_bounds.rend(); iter++){
                array_sizes.insert(array_sizes.begin(),total_size);
                total_size *= (*iter);
            }
            auto *array_type = ArrayType::get(var_type,total_size);
            Value *var;
            if (scope.in_global()) {
                auto initializer = ConstantZero::get(array_type, module.get());
                var = GlobalVariable::create(node.name,module.get(),array_type,false,initializer);
            }
            else {
                var = builder->create_alloca(array_type);
            }
            scope.push(node.name, var);
            scope.push_size(node.name, array_sizes);
            if (node.is_inited){
                cur_pos = 0;
                cur_depth = 0;
                node.initializers->accept(*this);
                for (int i = 0; i < array_bounds[0]; i++){
                    if (initval[i]){
                        builder->create_store(initval[i],builder->create_gep(var,{CONST_INT(0),CONST_INT(i)}));
                    }
                    else{
                        builder->create_store(CONST_INT(0),builder->create_gep(var,{CONST_INT(0),CONST_INT(i)}));
                    }
                }
            }
        }
    }
}

void MHSJbuilder::visit(SyntaxTree::AssignStmt &node){
    node.value->accept(*this);
    auto result = tmp_val;
    require_lvalue = true;
    node.target->accept(*this);
    auto addr = tmp_val;
    builder->create_store(result, addr);
    tmp_val = result;
}

void MHSJbuilder::visit(SyntaxTree::LVal &node){
    //FIXME:may have bug
    auto var = scope.find(node.name);
    bool should_return_lvalue = require_lvalue;
    require_lvalue = false;
    if (node.array_index.empty()){
        if (should_return_lvalue) {
            tmp_val = var;
            require_lvalue = false;
        }
        else {
            tmp_val = builder->create_load(var);
        }
    }
    else{
        auto var_sizes = scope.find_size(node.name);
        Value *var_index = nullptr;
        for (int i = 0; i < node.array_index.size(); i++){
            node.array_index[i]->accept(*this);
            auto index_val = tmp_val;
            auto exceptBB = BasicBlock::create(module.get(), "", cur_fun);
            auto contBB = BasicBlock::create(module.get(), "", cur_fun);
            Value *is_neg = builder->create_icmp_lt(index_val, CONST_INT(0));

            builder->create_cond_br(is_neg, exceptBB, contBB);
            builder->set_insert_point(exceptBB);
            auto neg_idx_except_fun = scope.find("neg_idx_except");
            builder->create_call( static_cast<Function *>(neg_idx_except_fun), {});
            if (cur_fun->get_return_type()->is_void_type())
                builder->create_void_ret();
            else
                builder->create_ret(CONST_INT(0));

            builder->set_insert_point(contBB);
            if (node.array_index.size()==1){
                //1维数组
                auto tmp_ptr = builder->create_gep(var, {CONST_INT(0), index_val});
                if (should_return_lvalue){
                    tmp_val = tmp_ptr;
                    require_lvalue = false;
                }
                else{
                    tmp_val = builder->create_load(tmp_ptr);
                }
            }
            else{
                //多维数组
                auto one_index = builder->create_imul(CONST_INT(var_sizes[i]),index_val);
                if (var_index == nullptr){
                    var_index = one_index;
                }
                else{
                    var_index = builder->create_iadd(var_index,one_index);
                }
            }
        }//end for
        if (node.array_index.size()>1){
            auto tmp_ptr = builder->create_gep(var, {CONST_INT(0), var_index});
            if (should_return_lvalue){
                tmp_val = tmp_ptr;
                require_lvalue = false;
            }
            else{
                tmp_val = builder->create_load(tmp_ptr);
            }
        }
    }
}

void MHSJbuilder::visit(SyntaxTree::Literal &node){
    tmp_val = CONST_INT(node.int_const);
}

void MHSJbuilder::visit(SyntaxTree::ReturnStmt &node){
    if (node.ret==nullptr){
        builder->create_void_ret();
    }
    else {
        auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
        node.ret->accept(*this);
        builder->create_ret(tmp_val);
    }
}

void MHSJbuilder::visit(SyntaxTree::BlockStmt &node){
    bool need_exit_scope = !pre_enter_scope;
    if (pre_enter_scope) {
        pre_enter_scope = false;
    }
    else {
        scope.enter();
    }
    for (auto& decl: node.body) {
        decl->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }
    if (need_exit_scope) {
        scope.exit();
    }
}

void MHSJbuilder::visit(SyntaxTree::EmptyStmt &node){
    
}

void MHSJbuilder::visit(SyntaxTree::ExprStmt &node){
    node.exp->accept(*this);
}

void MHSJbuilder::visit(SyntaxTree::UnaryCondExpr &node){
    if (node.op == SyntaxTree::UnaryCondOp::NOT){
        node.rhs->accept(*this);
        auto r_val = tmp_val;
        tmp_val = builder->create_icmp_ne(r_val, CONST_INT(0));
    }
}

void MHSJbuilder::visit(SyntaxTree::BinaryCondExpr &node){
    CmpInst *cond_val;
    if (node.op == SyntaxTree::BinaryCondOp::LAND) {
        auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
        node.lhs->accept(*this);
        auto ret_val = tmp_val;
        cond_val = dynamic_cast<CmpInst*>(ret_val);
        if (cond_val == nullptr) {
            cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
        }
        builder->create_cond_br(cond_val, trueBB, IF_While_Stack.back().falseBB);
        builder->set_insert_point(trueBB);
        node.rhs->accept(*this);
    } else if (node.op == SyntaxTree::BinaryCondOp::LOR) {
        auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
        node.lhs->accept(*this);
        auto ret_val = tmp_val;
        cond_val = dynamic_cast<CmpInst*>(ret_val);
        if (cond_val == nullptr) {
            cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
        }
        builder->create_cond_br(cond_val, IF_While_Stack.back().trueBB, falseBB);
        builder->set_insert_point(falseBB);
        node.rhs->accept(*this);
    } else {
        node.lhs->accept(*this);
        auto l_val = tmp_val;
        node.rhs->accept(*this);
        auto r_val = tmp_val;
        Value *cmp;
        switch (node.op) {
        case SyntaxTree::BinaryCondOp::LT:
            cmp = builder->create_icmp_lt(l_val, r_val);
            break;
        case SyntaxTree::BinaryCondOp::LTE:
            cmp = builder->create_icmp_le(l_val, r_val);
            break;
        case SyntaxTree::BinaryCondOp::GTE:
            cmp = builder->create_icmp_ge(l_val, r_val);
            break;
        case SyntaxTree::BinaryCondOp::GT:
            cmp = builder->create_icmp_gt(l_val, r_val);
            break;
        case SyntaxTree::BinaryCondOp::EQ:
            cmp = builder->create_icmp_eq(l_val, r_val);
            break;
        case SyntaxTree::BinaryCondOp::NEQ:
            cmp = builder->create_icmp_ne(l_val, r_val);
            break;
        }
        tmp_val = cmp;
    }
}

void MHSJbuilder::visit(SyntaxTree::BinaryExpr &node){
    if (node.rhs == nullptr){
        node.lhs->accept(*this);
    }
    else{
        node.rhs->accept(*this);
        auto r_val = tmp_val;
        node.lhs->accept(*thid);
        auto l_val = tmp_val;
        switch (node.op){
            case SyntaxTree::BinOp::PLUS:
                tmp_val = builder->create_iadd(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MINUS:
                tmp_val = builder->create_isub(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MULTIPLY:
                tmp_val = builder->create_imul(l_val, r_val);
                break;
            case SyntaxTree::BinOp::DIVIDE:
                tmp_val = builder->create_isdiv(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MODULO:
                tmp_val = builder->create_isrem(l_val, r_val);
        }
    }
}

void MHSJbuilder::visit(SyntaxTree::UnaryExpr &node){
    node.rhs->accept(*this);
    if (node.op == SyntaxTree::UnaryOp::MINUS){
        auto r_val = tmp_val;
        tmp_val = builder->create_isub(CONST_INT(0), r_val);
    }
}

void MHSJbuilder::visit(SyntaxTree::FuncCallStmt &node){
    auto fun = static_cast<Function *>(scope.find(node.name));
    std::vector<Value *> params;
    auto param_type = fun->get_function_type()->param_begin();
    for (auto &param: node.params) {
        param->accept(*this);
        params.push_back(tmp_val);
        param_type++;
    }
    tmp_val = builder->create_call(static_cast<Function *>(fun), params);
}

void MHSJbuilder::visit(SyntaxTree::IfStmt &node){
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    IF_While_Stack.back().trueBB = trueBB;
    if (node.else_statement == nullptr) {
        IF_While_Stack.back().falseBB = contBB;
    } else {
        IF_While_Stack.back().falseBB = falseBB;
    }
    node.cond_exp->accept(*this);
    auto ret_val = tmp_val;
    CmpInst *cond_val = dynamic_cast<CmpInst*>(ret_val);
    if (cond_val == nullptr) {
        cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    }
    if (node.else_statement == nullptr) {
        builder->create_cond_br(cond_val, trueBB, contBB);
    } else {
        builder->create_cond_br(cond_val, trueBB, falseBB);
    }
    builder->set_insert_point(trueBB);
    if (dynamic_cast<SyntaxTree::BlockStmt*>(node.if_statement.get())){
        node.if_statement->accept(*this);
    } else {
        scope.enter();
        node.if_statement->accept(*this);
        scope.exit();
    }

    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(contBB);

    if (node.else_statement == nullptr) {
        falseBB->erase_from_parent();
    } else {
        builder->set_insert_point(falseBB);
        if (dynamic_cast<SyntaxTree::BlockStmt*>(node.else_statement.get())){
            node.else_statement->accept(*this);
        } else {
            scope.enter();
            node.else_statement->accept(*this);
            scope.exit();
        }
        if (builder->get_insert_block()->get_terminator() == nullptr)
            builder->create_br(contBB);
    }

    builder->set_insert_point(contBB);
    IF_While_Stack.pop_back();
}

void MHSJbuilder::visit(SyntaxTree::WhileStmt &node){
    auto whileBB = BasicBlock::create(module.get(), "", cur_fun);
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    IF_While_Stack.push_back({trueBB, contBB});
    While_Stack.push_back({trueBB, contBB});
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(whileBB);
    builder->set_insert_point(whileBB);
    node.cond_exp->accept(*this);
    auto ret_val = tmp_val;    
    CmpInst *cond_val = dynamic_cast<CmpInst*>(ret_val);
    if (cond_val == nullptr) {
        cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    }
    builder->create_cond_br(cond_val, trueBB, contBB);
    builder->set_insert_point(trueBB);
    if (dynamic_cast<SyntaxTree::BlockStmt*>(node.statement.get())){
        node.statement->accept(*this);
    } else {
        scope.enter();
        node.statement->accept(*this);
        scope.exit();
    }
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(whileBB);
    builder->set_insert_point(contBB);
    IF_While_Stack.pop_back();
    While_Stack.pop_back();
}

void MHSJbuilder::visit(SyntaxTree::BreakStmt &node){
    builder->create_br(While_Stack.back().falseBB);
}

void MHSJbuilder::visit(SyntaxTree::ContinueStmt &node){
    builder->create_br(While_Stack.back().trueBB);
}
