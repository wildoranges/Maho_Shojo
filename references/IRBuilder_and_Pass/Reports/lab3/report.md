# lab3 实验报告
PB18111719 黄庄湫

## 问题1: cpp与.ll的对应
请描述你的cpp代码片段和.ll的每个BasicBlock的对应关系。描述中请附上两者代码。
1. assign_generator:
assign_hand.ll中只有entry一个BasicBlock,对应的即为assign_generator.cpp中main函数的整个代码块
<details>
  <summary>assign_generator.cpp</summary>

```cpp
  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  builder->set_insert_point(bb);

  auto retAlloca = builder->create_alloca(Int32Type);
  builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0

  auto *arrayType = ArrayType::get(Int32Type, 10);            // 分配数组
  auto a = builder->create_alloca(arrayType);
  // 获取a[0]和a[1]的地址
  auto a0GEP = builder->create_gep(a, {CONST_INT(0), CONST_INT(0)});
  auto a1GEP = builder->create_gep(a, {CONST_INT(0), CONST_INT(1)});
  builder->create_store(CONST_INT(10), a0GEP);
  auto a0Load = builder->create_load(a0GEP);
  auto mul = builder->create_imul(a0Load, CONST_INT(2));      // a[1] = a[0] * 2
  builder->create_store(mul, a1GEP);
  
  auto a1Load = builder->create_load(a1GEP);                  // return a[1]
  builder->create_store(a1Load, retAlloca);
  auto return_value = builder->create_load(retAlloca);
  builder->create_ret(return_value);
```

</details>

<details>
<summary>assign_hand.ll</summary>

```lex
define i32 @main() {
entry:
  %0 = alloca i32           ; initialize return_value
  store i32 0, i32* %0
  %1 = alloca [10 x i32]    ; initialize array a[10]
  ; %2 = &a[0]
  %2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 0
  ; %3 = &a[1]
  %3 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 1
  store i32 10, i32* %2     ; a[0] = 10
  %4 = load i32, i32* %2
  %5 = mul i32 %4, 2        ; a[1] = a[0] * 2
  store i32 %5, i32* %3
  %6 = load i32, i32* %3
  store i32 %6, i32* %0     ; return_value = a[1]
  %7 = load i32, i32* %0
  ret i32 %7
}
```

</details>

2. fun_generator:
fun_hand.ll中有两个函数,main和callee,每个函数中只有一个BasicBlock,对应的即为fun_generator.cpp中的两个Function及其含有的BasicBlock
<details>
<summary>fun_generator.cpp</summary>

```cpp
  // callee函数
  // 函数参数类型的vector
  std::vector<Type *> Ints(1, Int32Type);

  //通过返回值类型与参数类型列表得到函数类型
  auto calleeFunTy = FunctionType::get(Int32Type, Ints);
  // 由函数类型得到函数
  auto calleeFun = Function::create(calleeFunTy, "callee", module);

  auto bb = BasicBlock::create(module, "entry", calleeFun);
  builder->set_insert_point(bb);                        // 一个BB的开始,将当前插入指令点的位置设在bb

  auto retAlloca = builder->create_alloca(Int32Type);   // 在内存中分配返回值的位置
  auto aAlloca = builder->create_alloca(Int32Type);     // 在内存中分配参数a的位置

  std::vector<Value *> args;  // 获取callee函数的形参,通过Function中的iterator
  for (auto arg = calleeFun->arg_begin(); arg != calleeFun->arg_end(); arg++) {
    args.push_back(*arg);   // * 号运算符是从迭代器中取出迭代器当前指向的元素
  }

  builder->create_store(args[0], aAlloca);                // 将参数a store下来
  auto aLoad = builder->create_load(aAlloca);             // 将参数a load上来
  auto mul = builder->create_imul(CONST_INT(2), aLoad);

  builder->create_store(mul, retAlloca);                  // return 2 * a
  auto return_value = builder->create_load(retAlloca);
  builder->create_ret(return_value);
```

```cpp
  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
  bb = BasicBlock::create(module, "entry", mainFun);

  builder->set_insert_point(bb);

  retAlloca = builder->create_alloca(Int32Type);
  builder->create_store(CONST_INT(0), retAlloca);                 // 默认 ret 0

  auto call = builder->create_call(calleeFun, {CONST_INT(110)});  // call func callee
  builder->create_store(call, retAlloca);                         // store callee's return value
  return_value = builder->create_load(retAlloca);                 // return
  builder->create_ret(return_value);
```

</details>

<details>
<summary>fun_hand.ll</summary>

```lex
define i32 @callee(i32 %0) {
entry:
  %1 = alloca i32                 ; initialize return_value
  %2 = alloca i32                 ; %2 = &a
  store i32 %0, i32* %2           ; initialize a
  %3 = load i32, i32* %2
  %4 = mul i32 2, %3              ; %4 = 2 * a
  store i32 %4, i32* %1           ; store return_value
  %5 = load i32, i32* %1          ; return
  ret i32 %5
}
```

```lex
define i32 @main() {
entry:
  %0 = alloca i32                 ; initialize return_value
  store i32 0, i32* %0
  %1 = call i32 @callee(i32 110)  ; call func callee
  store i32 %1, i32* %0           ; store callee's return_value
  %2 = load i32, i32* %0          ; return
  ret i32 %2
}
```

</details>

3. if_generator:
if_hand.ll中有4个BasicBlock,分别对应if_generator.cpp的main函数中进入if分支前的部分(entry),if条件为真的分支(trueBB),if条件为假的分支(falseBB),以及最后结束main函数的(end_entry)
<details>
<summary>if_generator.cpp</summary>

```cpp
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
```

```cpp
  builder->set_insert_point(trueBB);
  builder->create_store(CONST_INT(233), retAlloca);               // return 233
  return_value = builder->create_load(retAlloca);
  builder->create_ret(return_value);
```

```cpp
  builder->set_insert_point(falseBB);
  builder->create_br(end_entry);  
```

```cpp
  auto end_entry = BasicBlock::create(module, "end_entry", mainFun);
  builder->set_insert_point(end_entry);
  auto return_value = builder->create_load(retAlloca);            // return 0
  builder->create_ret(return_value);
```

</details>

<details>
<summary>if_hand.ll</summary>

```lex
entry:
  %0 = alloca i32           ; initialize return_value
  store i32 0, i32* %0
  %1 = alloca float         ; initialize a = 5.555
  store float 0x40163851e0000000, float* %1
  %2 = load float, float* %1
  %3 = fcmp ugt float %2,0x3ff0000000000000   ; (a > 1)?
  br i1 %3, label %trueBB, label %falseBB
```

```lex
trueBB:
  store i32 233, i32* %0    ; return 233
  %4 = load i32, i32* %0
  ret i32 %4
```

```lex
falseBB:
  br label %end_entry
```

```lex
end_entry:
  %5 = load i32, i32* %0  ; return 0
  ret i32 %5
```

</details>

4. while_generator:
while_hand.ll有5个BasicBlock,对应while_generator.cpp的main函数中进入while循环之前的部分(entry),while循环体头部对于条件的判断(while_entry),while循环体中条件为真的分支(trueBB),条件为假的分支(falseBB),以及退出循环体后结束main函数的部分(end_entry)
<details>
<summary>while_generator.cpp</summary>

```cpp
  auto bb = BasicBlock::create(module, "entry", mainFun);
  auto bb_while = BasicBlock::create(module, "while_entry", mainFun);

  builder->set_insert_point(bb);

  auto retAlloca = builder->create_alloca(Int32Type);
  builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0

  auto a = builder->create_alloca(Int32Type);
  auto i = builder->create_alloca(Int32Type);

  builder->create_store(CONST_INT(10), a);                    // a = 10
  builder->create_store(CONST_INT(0), i);                     // i = 0

  builder->create_br(bb_while);                               // 进入while循环体
```

```cpp
  builder->set_insert_point(bb_while);

  auto iLoad = builder->create_load(i);
  auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));      // (i < 10)?
  auto trueBB = BasicBlock::create(module, "trueBB", mainFun);
  auto falseBB = BasicBlock::create(module, "falseBB", mainFun);
  builder->create_cond_br(icmp, trueBB, falseBB);
```

```cpp
  auto end_entry = BasicBlock::create(module, "end_entry", mainFun);
  builder->set_insert_point(end_entry);
  auto aLoad = builder->create_load(a);                       // return a
  builder->create_store(aLoad, retAlloca);
  auto return_value = builder->create_load(retAlloca);
  builder->create_ret(return_value);
```

```cpp
  builder->set_insert_point(trueBB);
  auto add = builder->create_iadd(iLoad, CONST_INT(1));           // i = i + 1
  builder->create_store(add, i);
  aLoad = builder->create_load(a);                                // a = a + i
  add = builder->create_iadd(aLoad, add);
  builder->create_store(add, a);
  builder->create_br(bb_while);  
```

```cpp
  builder->set_insert_point(falseBB);
  builder->create_br(end_entry);  
```

</details>

<details>
<summary>while_hand.ll</summary>

```lex
entry:
  %0 = alloca i32           ; initialize return_value
  store i32 0, i32* %0
  %1 = alloca i32           ; %1 = &a
  %2 = alloca i32           ; %2 = &i
  store i32 10, i32* %1     ; a = 10
  store i32 0, i32* %2      ; i = 0
  br label %while_entry
```

```lex
while_entry:
  %3 = load i32, i32* %2    ; (i < 10)?
  %4 = icmp slt i32 %3, 10
  br i1 %4, label %trueBB, label %falseBB
```

```lex
end_entry:
  %5 = load i32, i32* %1    ; load a
  store i32 %5, i32* %0     ; store a to &return_value
  %6 = load i32, i32* %0    ; return
  ret i32 %6
```

```lex
trueBB:
  %7 = add i32 %3, 1        ; i = i + 1
  store i32 %7, i32* %2
  %8 = load i32, i32* %1    ; a = a + i
  %9 = add i32 %8, %7
  store i32 %9, i32* %1
  br label %while_entry     ; loop
```

```lex
falseBB:
  br label %end_entry       ; end
```

</details>

## 问题2: Visitor Pattern
请指出visitor.cpp中，`treeVisitor.visit(exprRoot)`执行时，以下几个Node的遍历序列:numberA、numberB、exprC、exprD、exprE、numberF、exprRoot。  
序列请按如下格式指明：  
exprRoot->numberF->exprE->numberA->exprD

遍历序列为:
exprRoot->numberF->exprRoot->exprE->exprD->numberB->exprD->numberA->exprD->exprE->exprC->numberA->exprC->numberB->exprC->exprE->exprRoot

## 问题3: getelementptr
请给出`IR.md`中提到的两种getelementptr用法的区别,并稍加解释:
  - `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0` 
  - `%2 = getelementptr i32, i32* %1, i32 %0` 

`%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0` 执行的结果是`%2中存放的是%1指向的数组中第%0个元素的地址`(`%2 = &%1(0)[%0]`)
`%2 = getelementptr i32, i32* %1, i32 %0` 执行的结果是`%2中存放的是在%1指向的元素的地址之后%0*sizeof(i32)的地址`(`%2 = &%1(%0)`)
getelementptr返回对一个聚合体取给定索引的地址,使用命令时，该聚合体的地址位于第二个参数，并且以该聚合体类型的指针修饰。而第一个参数即是聚合体的类型。而第三个参数是第二个指针参数自身的偏移,而从第四个参数开始（如果有的话，没有默认为零），才是关于该聚合体内部结构的索引（数组下标的索引或者结构体成员的索引）。
第一条指令中聚合体的类型为`int()[10]`,`i32 0`是指取`int()[10]`的0倍单位的偏移,`i32 %0`是指取`int`的`%0`倍单位的偏移。要索引到数组内特定地址,先要找到`(%1)+0*sizeof(int()[10])`,即指向与当前数组偏移0个数组大小的"数组"的首地址,再在该"数组"内部使用`i32 %0`找到数组内的偏移量并最终返回该索引的地址。
第二条指令中聚合体的元素类型为`int()`,`i32 %0`是指取`int()`的`%0`倍单位的偏移,即寻找`(%1)+%0*sizeof(int)`,即`%1`往后偏移`%0`个int大小的位置,并将其赋给`%2`。

## 实验难点
描述在实验中遇到的问题、分析和解决方案
本次实验没有遇到什么问题,我在助教的文档和同学们发的issues的帮助下顺利完成了实验

## 实验反馈
助教可以设计更复杂的测试样例作为选做,供学有余力的同学们完成