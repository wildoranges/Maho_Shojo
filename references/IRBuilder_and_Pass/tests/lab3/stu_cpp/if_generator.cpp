#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
  auto module = new Module("Cminus code");
  auto builder = new IRBuilder(nullptr, module);
  Type *Int32Type = Type::get_int32_type(module);
  Type *FloatType = Type::get_float_type(module);

  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);

  builder->set_insert_point(bb);

  auto retAlloca = builder->create_alloca(Int32Type);
  builder->create_store(CONST_INT(0), retAlloca);                 // 默认 ret 0

  auto a = builder->create_alloca(FloatType);

  builder->create_store(CONST_FP(5.555), a);                      // a = 5.555
  auto aLoad = builder->create_load(a);
  auto fcmp = builder->create_fcmp_gt(aLoad, CONST_FP(1.0));      // (a > 1)?
  auto trueBB = BasicBlock::create(module, "trueBB", mainFun);
  auto falseBB = BasicBlock::create(module, "falseBB", mainFun);
  builder->create_cond_br(fcmp, trueBB, falseBB);
  
  auto end_entry = BasicBlock::create(module, "end_entry", mainFun);
  builder->set_insert_point(end_entry);
  auto return_value = builder->create_load(retAlloca);            // return 0
  builder->create_ret(return_value);

  builder->set_insert_point(trueBB);
  builder->create_store(CONST_INT(233), retAlloca);               // return 233
  return_value = builder->create_load(retAlloca);
  builder->create_ret(return_value);

  builder->set_insert_point(falseBB);
  builder->create_br(end_entry);                                  // end

  std::cout << module->print();
  delete module;
  return 0;
}