/*
 * 声明：本代码为 2020 秋 中国科大编译原理（李诚）课程实验参考实现。
 * 请不要以任何方式，将本代码上传到可以公开访问的站点或仓库
 */

#include "MHSJBuilder.hpp"

#define CONST_INT(num) ConstantInt::get(num, module.get())

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

/* Global Variable */

// used for backpatching
struct true_false_BB {
  BasicBlock *trueBB = nullptr;
  BasicBlock *falseBB = nullptr;
};

std::list<true_false_BB> IF_While_Stack; // used for Cond
std::list<true_false_BB> While_Stack;    // used for break and continue
// used for backpatching
std::vector<SyntaxTree::FuncParam> func_fparams;
std::vector<int> array_bounds;
std::vector<int> array_sizes;
int cur_pos;
int cur_depth;
std::map<int, Value *> initval;
std::vector<Constant *> init_val;
/* Global Variable */

void MHSJBuilder::visit(SyntaxTree::Assembly &node) {
  VOID_T = Type::get_void_type(module.get());
  INT1_T = Type::get_int1_type(module.get());
  INT32_T = Type::get_int32_type(module.get());
  INT32PTR_T = Type::get_int32_ptr_type(module.get());
  for (const auto& def : node.global_defs) {
    def->accept(*this);
  }
}

void MHSJBuilder::visit(SyntaxTree::InitVal &node) {
  if (node.isExp) {
    node.expr->accept(*this);
    initval[cur_pos] = tmp_val;
    init_val.push_back(dynamic_cast<Constant *>(tmp_val));
    cur_pos++;
  }
  else {
    for (const auto& elem : node.elementList) {
      if (cur_depth!=0){
          while (cur_pos % array_sizes[cur_depth] != 0) {
          init_val.push_back(CONST_INT(0));
          cur_pos++;
        }
      }
      cur_depth++;
      elem->accept(*this);
      cur_depth--;
      if (cur_depth!=0){
        while (cur_pos % array_sizes[cur_depth] != array_sizes[cur_depth] - 1) {
          init_val.push_back(CONST_INT(0));
          cur_pos++;
        }
      }
    }
    if (cur_depth==0){
      while (cur_pos < array_sizes[0]){
        init_val.push_back(CONST_INT(0));
        cur_pos++;
      }
    }
  }
}

void MHSJBuilder::visit(SyntaxTree::FuncDef &node) {
  FunctionType *fun_type;
  Type *ret_type;
  if (node.ret_type == SyntaxTree::Type::INT)
    ret_type = INT32_T;
  else
    ret_type = VOID_T;

  std::vector<Type *> param_types;
  std::vector<SyntaxTree::FuncParam>().swap(func_fparams);
  node.param_list->accept(*this);
  for (const auto& param : func_fparams) {
    if (param.param_type == SyntaxTree::Type::INT) {
      if (param.array_index.empty()) {
        param_types.push_back(INT32_T);
      } else {
        param_types.push_back(INT32PTR_T);
      }
    }
  }
  fun_type = FunctionType::get(ret_type, param_types);
  auto fun = Function::create(fun_type, node.name, module.get());
  scope.push(node.name, fun);
  cur_fun = fun;
  auto funBB = BasicBlock::create(module.get(), "entry", fun);
  builder->set_insert_point(funBB);
  scope.enter();
  pre_enter_scope = true;
  std::vector<Value *> args;
  for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++) {
    args.push_back(*arg);
  }
  for (int i = 0; i < func_fparams.size(); i++) {
    if (func_fparams[i].array_index.empty()) {
      Value *alloc;
      alloc = builder->create_alloca(INT32_T);
      builder->create_store(args[i], alloc);
      scope.push(func_fparams[i].name, alloc);
    } else {
      Value *alloc_array;
      alloc_array = builder->create_alloca(INT32PTR_T);
      builder->create_store(args[i], alloc_array);
      scope.push(func_fparams[i].name, alloc_array);
    }
  }
  node.body->accept(*this);
  if (builder->get_insert_block()->get_terminator() == nullptr) {
    if (cur_fun->get_return_type()->is_void_type())
      builder->create_void_ret();
    else
      builder->create_ret(CONST_INT(0));
  }
  scope.exit();
}

void MHSJBuilder::visit(SyntaxTree::FuncFParamList &node) {
  for (const auto& Param : node.params) {
    Param->accept(*this);
  }
}

void MHSJBuilder::visit(SyntaxTree::FuncParam &node) {
  func_fparams.push_back(node);
}

void MHSJBuilder::visit(SyntaxTree::VarDef &node) {
  Type *var_type;
  if (node.is_constant && 0) {
    // TODO:constant
  } else {
    var_type = INT32_T;
    if (node.array_length.empty()) {
      Value *var;
      if (scope.in_global()) {
        if (node.is_inited) {
          node.initializers->accept(*this);
          auto initializer = dynamic_cast<ConstantInt *>(tmp_val);
          var = GlobalVariable::create(node.name, module.get(), var_type, false, initializer);
        }
        else{
          auto initializer = ConstantZero::get(var_type, module.get());
          var = GlobalVariable::create(node.name, module.get(), var_type, false, initializer);
        }
        scope.push(node.name, var);
      } else {
        var = builder->create_alloca(var_type);
        scope.push(node.name, var);
        if (node.is_inited) {
          node.initializers->accept(*this);
          builder->create_store(tmp_val, var);
        }
      }
    } else {
      // array
      array_bounds.clear();
      array_sizes.clear();
      for (const auto& bound_expr : node.array_length) {
        bound_expr->accept(*this);
        auto bound_const = dynamic_cast<ConstantInt *>(tmp_val);
        auto bound = bound_const->get_value();
        array_bounds.push_back(bound);
      }
      int total_size = 1;
      for (auto iter = array_bounds.rbegin(); iter != array_bounds.rend();
           iter++) {
        array_sizes.insert(array_sizes.begin(), total_size);
        total_size *= (*iter);
      }
      array_sizes.insert(array_sizes.begin(), total_size);
      auto *array_type = ArrayType::get(var_type, total_size);

      Value *var;
      if (scope.in_global()) {
        if (node.is_inited ){
          cur_pos = 0;
          cur_depth = 0;
          init_val.clear();
          node.initializers->accept(*this);
          auto initializer = ConstantArray::get(array_type, init_val);
          var = GlobalVariable::create(node.name, module.get(), array_type, false, initializer);
          scope.push(node.name, var);
          scope.push_size(node.name, array_sizes);
        }
        else {
          auto initializer = ConstantZero::get(array_type, module.get());
          var = GlobalVariable::create(node.name, module.get(), array_type, false, initializer);
          scope.push(node.name, var);
          scope.push_size(node.name, array_sizes);
        }
      }
      else {
        var = builder->create_alloca(array_type);
        scope.push(node.name, var);
        scope.push_size(node.name, array_sizes);
        if (node.is_inited) {
          cur_pos = 0;
          cur_depth = 0;
          initval.clear();
          node.initializers->accept(*this);
          for (int i = 0; i < array_bounds[0]; i++) {
            if (initval[i]) {
              builder->create_store(initval[i], builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
            } else {
              builder->create_store(CONST_INT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
            }
          }
        }
      }//if of global check
    }
  }
}

void MHSJBuilder::visit(SyntaxTree::AssignStmt &node) {
  node.value->accept(*this);
  auto result = tmp_val;
  require_lvalue = true;
  node.target->accept(*this);
  auto addr = tmp_val;
  builder->create_store(result, addr);
  tmp_val = result;
}

void MHSJBuilder::visit(SyntaxTree::LVal &node) {
  // FIXME:may have bug
  auto var = scope.find(node.name);
  bool should_return_lvalue = require_lvalue;
  require_lvalue = false;
  if (node.array_index.empty()) {
    if (should_return_lvalue) {
      tmp_val = var;
      require_lvalue = false;
    } else {
      tmp_val = builder->create_load(var);
    }
  } else {
    auto var_sizes = scope.find_size(node.name);
    Value *var_index = nullptr;
    for (int i = 0; i < node.array_index.size(); i++) {
      node.array_index[i]->accept(*this);
      auto index_val = tmp_val;
      if (node.array_index.size() == 1) {
        // 1维数组
        auto tmp_ptr = builder->create_gep(var, {CONST_INT(0), index_val});
        if (should_return_lvalue) {
          tmp_val = tmp_ptr;
          require_lvalue = false;
        } else {
          tmp_val = builder->create_load(tmp_ptr);
        }
      } else {
        //多维数组
        auto one_index =
            builder->create_imul(CONST_INT(var_sizes[i + 1]), index_val);
        if (var_index == nullptr) {
          var_index = one_index;
        } else {
          var_index = builder->create_iadd(var_index, one_index);
        }
      }
    } // end for
    if (node.array_index.size() > 1) {
      auto tmp_ptr = builder->create_gep(var, {CONST_INT(0), var_index});
      if (should_return_lvalue) {
        tmp_val = tmp_ptr;
        require_lvalue = false;
      } else {
        tmp_val = builder->create_load(tmp_ptr);
      }
    }
  }
}

void MHSJBuilder::visit(SyntaxTree::Literal &node) {
  tmp_val = CONST_INT(node.int_const);
}

void MHSJBuilder::visit(SyntaxTree::ReturnStmt &node) {
  if (node.ret == nullptr) {
    builder->create_void_ret();
  } else {
    //auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
    node.ret->accept(*this);
    builder->create_ret(tmp_val);
  }
}

void MHSJBuilder::visit(SyntaxTree::BlockStmt &node) {
  bool need_exit_scope = !pre_enter_scope;
  if (pre_enter_scope) {
    pre_enter_scope = false;
  } else {
    scope.enter();
  }
  for (auto &decl : node.body) {
    decl->accept(*this);
    if (builder->get_insert_block()->get_terminator() != nullptr)
      break;
  }
  if (need_exit_scope) {
    scope.exit();
  }
}

void MHSJBuilder::visit(SyntaxTree::EmptyStmt &node) {}

void MHSJBuilder::visit(SyntaxTree::ExprStmt &node) { node.exp->accept(*this); }

void MHSJBuilder::visit(SyntaxTree::UnaryCondExpr &node) {
  if (node.op == SyntaxTree::UnaryCondOp::NOT) {
    node.rhs->accept(*this);
    auto r_val = tmp_val;
    tmp_val = builder->create_icmp_ne(r_val, CONST_INT(0));
  }
}

void MHSJBuilder::visit(SyntaxTree::BinaryCondExpr &node) {
  CmpInst *cond_val;
  if (node.op == SyntaxTree::BinaryCondOp::LAND) {
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    node.lhs->accept(*this);
    auto ret_val = tmp_val;
    cond_val = dynamic_cast<CmpInst *>(ret_val);
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
    cond_val = dynamic_cast<CmpInst *>(ret_val);
    if (cond_val == nullptr) {
      cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    }
    builder->create_cond_br(cond_val, IF_While_Stack.back().trueBB, falseBB);
    builder->set_insert_point(falseBB);
    node.rhs->accept(*this);
  } else {
    node.lhs->accept(*this);
    auto l_val = tmp_val;
    if (dynamic_cast<CmpInst*>(l_val)) {
      l_val = builder->create_zext(l_val, INT32_T);
    }
    node.rhs->accept(*this);
    auto r_val = tmp_val;
    if (dynamic_cast<CmpInst*>(r_val)) {
      r_val = builder->create_zext(r_val, INT32_T);
    }
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

void MHSJBuilder::visit(SyntaxTree::BinaryExpr &node) {
  if (node.rhs == nullptr) {
    node.lhs->accept(*this);
  } else {
    node.rhs->accept(*this);
    auto r_val = tmp_val;
    node.lhs->accept(*this);
    auto l_val = tmp_val;
    switch (node.op) {
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

void MHSJBuilder::visit(SyntaxTree::UnaryExpr &node) {
  node.rhs->accept(*this);
  if (node.op == SyntaxTree::UnaryOp::MINUS) {
    auto r_val = tmp_val;
    tmp_val = builder->create_isub(CONST_INT(0), r_val);
  }
}

void MHSJBuilder::visit(SyntaxTree::FuncCallStmt &node) {
  auto fun = static_cast<Function *>(scope.find(node.name));//FIXME:STATIC OR DYNAMIC?
  std::vector<Value *> params;
  auto param_type = fun->get_function_type()->param_begin();
  for (auto &param : node.params) {
    param->accept(*this);
    params.push_back(tmp_val);
    param_type++;
  }
  tmp_val = builder->create_call(static_cast<Function *>(fun), params);
}

void MHSJBuilder::visit(SyntaxTree::IfStmt &node) {
  auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
  auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
  auto contBB = BasicBlock::create(module.get(), "", cur_fun);
  IF_While_Stack.push_back({nullptr, nullptr});
  IF_While_Stack.back().trueBB = trueBB;
  if (node.else_statement == nullptr) {
    IF_While_Stack.back().falseBB = contBB;
  } else {
    IF_While_Stack.back().falseBB = falseBB;
  }
  node.cond_exp->accept(*this);
  auto ret_val = tmp_val;
  auto *cond_val = dynamic_cast<CmpInst *>(ret_val);
  if (cond_val == nullptr) {
    cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
  }
  if (node.else_statement == nullptr) {
    builder->create_cond_br(cond_val, trueBB, contBB);
  } else {
    builder->create_cond_br(cond_val, trueBB, falseBB);
  }
  builder->set_insert_point(trueBB);
  if (dynamic_cast<SyntaxTree::BlockStmt *>(node.if_statement.get())) {
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
    if (dynamic_cast<SyntaxTree::BlockStmt *>(node.else_statement.get())) {
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

void MHSJBuilder::visit(SyntaxTree::WhileStmt &node) {
  auto whileBB = BasicBlock::create(module.get(), "", cur_fun);
  auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
  auto contBB = BasicBlock::create(module.get(), "", cur_fun);
  IF_While_Stack.push_back({trueBB, contBB});
  While_Stack.push_back({whileBB, contBB});
  if (builder->get_insert_block()->get_terminator() == nullptr)
    builder->create_br(whileBB);
  builder->set_insert_point(whileBB);
  node.cond_exp->accept(*this);
  auto ret_val = tmp_val;
  auto *cond_val = dynamic_cast<CmpInst *>(ret_val);
  if (cond_val == nullptr) {
    cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
  }
  builder->create_cond_br(cond_val, trueBB, contBB);
  builder->set_insert_point(trueBB);
  if (dynamic_cast<SyntaxTree::BlockStmt *>(node.statement.get())) {
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

void MHSJBuilder::visit(SyntaxTree::BreakStmt &node) {
  builder->create_br(While_Stack.back().falseBB);
}

void MHSJBuilder::visit(SyntaxTree::ContinueStmt &node) {
  builder->create_br(While_Stack.back().trueBB);
}
