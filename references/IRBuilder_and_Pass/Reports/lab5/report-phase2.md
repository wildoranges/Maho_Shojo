# Lab5 实验报告

组长 曾源 PB18111741
组员 黄庄湫 PB18111719

## 实验要求

$\quad$掌握如何开发基于LightIR的优化Pass,要求实现包括```常量传播```，```循环不变式外提```与```活跃变量分析```在内的基本优化Pass。
* ```常量传播``` 能够实现在编译优化阶段，能够计算出结果的变量，就直接替换为常量.同时只需要考虑过程内的常量传播，可以不用考虑数组，全局变量只需要考虑块内的常量传播;整形浮点型都需要考虑;对于```a=1/0```的情形，可以不考虑，即可以做处理也可以不处理;做到删除无用的分支将条件跳转变换为强制性跳转;同时对于分支嵌套的情况都能够删除掉无用的分支，这一步之后对于可能出现的无法到达的块都需要进行删除，而至于只含有跳转指令的冗余块在本次实验中不要求实现。
* ```循环不变式外提``` 要能够实现将与循环无关的表达式提取到循环的外面。不用考虑数组，与全局变量。
* ```活跃变量分析``` 能够实现分析bb块的入口和出口的活跃变量,在```ActiveVars.hpp```中定义了两个成员```live_in, live_out```，需要将```pair<bb, IN[bb]>```插入```live_in```的map 结构中，将```pair<bb, OUT[bb]>```插入```live_out ```的map 结构中，并调用```ActiveVars类```中的```print()```方法输出bb活跃变量情况到json文件.

## 实验难点


*   常量传播：
$\quad$常量传播部分的主要难点为删除冗余分支时对后继块中phi指令的修改。大多数指令的常量折叠操作都很类似，没什么难度，而对常量全局变量表的更新与维护和load与store指令的处理难度也不大，尤其是助教不要求数组和块间全局变量的常量传播后难度骤减，因此主要难点为删除冗余分支的操作。

*   循环不变式外提：
$\quad$循环不变式外提最大的难点在于如何确定循环不变式，不过由于不需要考虑数组，全局变量等问题，在只考虑单个局部变量的情况下，这个问题能够通过遍历比较的方法较为轻松地解决。至于前驱块的确定则是通过助教提供的接口`get_inner_loop`便能轻松解决。

*   活跃变量分析：
$\quad$活跃变量分析的主要难点是对于数据流方程的修改：由于phi指令的存在，计算OUT集合的时候需要剔除后继块的phi指令中与当前块无关的变量。由于IR是SSA形式，因此def和use集合的计算变得相对容易，再加上因为$IN_{迭代前}\subseteq{IN_{迭代后}}$，$OUT_{迭代前}\subseteq{OUT_{迭代后}}$(归纳法易证)，所以对于IN和OUT集合的更新以及是否需要再次迭代的判断也变得更加容易,因此主要挑战就是数据流方程的更新。

## 实验设计

* $\textbf{常量传播}$
    $\textbf{实现思路}$ 常量传播部分的实现主要有2个方面：传统的常量传播操作(块内)；以及后续删除无用的分支，将条件跳转变换为强制性跳转。
    $\textbf{(1)传统的常量传播操作(通过const_propagation()实现)：}$
    $\quad$对于一条指令，根据指令的类型执行对应的操作。$\textbf{通用操作}$是获取指令的操作数，若都为常量，则根据指令类型和操作数类型调用`compute()`函数计算出表达式的值，并将所有对该指令计算结果的引用全部替换为`compute()`函数计算出的常量(通过`replace_all_use_with()`实现)，最终删去该指令。对于$\textbf{一般}$的指令，使用上述的通用操作即可(以整型值四则运算的常量折叠为例)

    <details><summary>$相应代码$</summary>

    ```cpp
    if (instr->is_add() || instr->is_sub() || instr->is_mul() || instr->is_div()) {
        auto value1 = cast_constantint(instr->get_operand(0));
        auto value2 = cast_constantint(instr->get_operand(1));
        if (value1 && value2) {
            auto folder_const = folder->compute(instr->get_instr_type(), value1, value2);
            instr->replace_all_use_with(folder_const);
            wait_delete.push_back(instr);
        }
    }
    ```
    </details>

    $\textbf{(1.1)load指令的处理}$
    $\quad$若指令的类型为`load`，则在块内维护一张`常量全局变量表`(若一个全局变量为常量，则该全局变量及其对应的右值在表中)中查找左值(`l_val`)(通过`get_global_const_val()`函数实现)，若左值在该表中，则将该左值对应的右值取出并替换所有对于该指令结果的引用；若指令的类型为`store`，并且右值为常量，则在该块的`常量全局变量表`中将左值(`l_val`)对应的右值更新为`store`指令中得到的常量右值(通过`set_global_const_val()`函数实现)，若右值不为常量，则删去表中的左值(对于局部变量的`load`和`store`指令，已在`Mem2Reg`中优化完成，并且助教不要求对数组的分析，故常量传播部分的`load`和`store`指令只对全局变量进行分析)

    <details><summary>$相应代码$</summary>

    ```cpp
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
    ```
    </details>

    <details><summary>get_global_const_val()和set_global_const_val()</summary>

    ```cpp
    // 可同时处理整型值和浮点值的全局变量
    Constant *ConstPropagation::get_global_const_val(Value *value) {
        auto global_value = dynamic_cast<GlobalVariable *>(value);
        if (global_value != nullptr) {
            if (const_global_var.find(value) != const_global_var.end()) {
                auto iter = const_global_var.find(value);
            return iter->second;
            }
        }
        return nullptr;
    }

    // 可同时处理整型值和浮点值的全局变量
    Constant *ConstPropagation::set_global_const_val(Value *value, Constant *const_val) {
        auto global_value = dynamic_cast<GlobalVariable *>(value);
        if (global_value != nullptr) {
            if (const_global_var.find(global_value) != const_global_var.end()) {
                auto iter = const_global_var.find(global_value);
                iter->second = const_val;
                return const_val;
            }
            else {
                const_global_var.insert({global_value, const_val});
                return const_val;
            }
        }
        return nullptr;
    }
    ```
    </details>
    $\textbf{(1.2)ret、call、gep指令的处理}$

    $\quad$ 若指令类型为`ret`，`call`，`gep`，则不需要进行常量传播的操作(助教的不要求考虑数组的常量传播，故没有实现`gep`指令的常量传播；`ret`指令无左值，后续不可能有再引用左值的地方，无法减少实际执行的指令数，故常量传播操作对于`ret`指令而言无意义；`call`指令的参数传递使用常量传播无意义，不能减少执行的指令条数，而函数调用的返回结果很难通过静态分析得出，并且函数调用的返回结果的常量传播为过程间常量传播，助教不要求，故最终没有实现`call`指令的常量传播)。
    $\textbf{(2)删除无用分支，将条件跳转变换为强制跳转(reduce_redundant_cond_br()实现)：}$
    $\quad$由于每个基本块的最后一条语句必定为终结语句(`ret`或`br`)，`ret`或`br`语句也只可能出现在基本块的最后一条语句，所以删除冗余条件分支时，只需要判断最后一条语句(通过`get_terminator()函数`获取)是否为`cond_br`类型(条件跳转)，若是条件跳转指令，则再判断条件是否为常量，若为常量，则根据条件的真假决定哪一个分支存活。对于当前基本块的后继块，若该后继块不是先前决定的存活分支，则删去与当前基本块的前驱关系,同时删除其后继块`phi`指令中与当前基本块有关的部分(如果有`phi`指令的话)。最后删除当前条件跳转指令，创建一条跳转到存活分支的强制跳转指令，并更新当前基本块的后继块集合。

    <details><summary>$相应代码$</summary>

    ```cpp
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
    ```
    </details>

    $\textbf{优化前后的IR对比}$
    以`testcase-3.cminus`的优化为例:

    <details><summary>开启常量传播优化前</summary>

    ```llvm
    ; ModuleID = 'cminus'
    source_filename = "ConstPropagation/testcase-3.cminus"

    @opa = global i32 zeroinitializer
    @opb = global i32 zeroinitializer
    @opc = global i32 zeroinitializer
    @opd = global i32 zeroinitializer
    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define i32 @max() {
    label_entry:
    %op0 = mul i32 0, 1
    %op1 = mul i32 %op0, 2
    %op2 = mul i32 %op1, 3
    %op3 = mul i32 %op2, 4
    %op4 = mul i32 %op3, 5
    %op5 = mul i32 %op4, 6
    %op6 = mul i32 %op5, 7
    store i32 %op6, i32* @opa
    %op7 = mul i32 1, 2
    %op8 = mul i32 %op7, 3
    %op9 = mul i32 %op8, 4
    %op10 = mul i32 %op9, 5
    %op11 = mul i32 %op10, 6
    %op12 = mul i32 %op11, 7
    %op13 = mul i32 %op12, 8
    store i32 %op13, i32* @opb
    %op14 = mul i32 2, 3
    %op15 = mul i32 %op14, 4
    %op16 = mul i32 %op15, 5
    %op17 = mul i32 %op16, 6
    %op18 = mul i32 %op17, 7
    %op19 = mul i32 %op18, 8
    %op20 = mul i32 %op19, 9
    store i32 %op20, i32* @opc
    %op21 = mul i32 3, 4
    %op22 = mul i32 %op21, 5
    %op23 = mul i32 %op22, 6
    %op24 = mul i32 %op23, 7
    %op25 = mul i32 %op24, 8
    %op26 = mul i32 %op25, 9
    %op27 = mul i32 %op26, 10
    store i32 %op27, i32* @opd
    %op28 = load i32, i32* @opa
    %op29 = load i32, i32* @opb
    %op30 = icmp slt i32 %op28, %op29
    %op31 = zext i1 %op30 to i32
    %op32 = icmp ne i32 %op31, 0
    br i1 %op32, label %label33, label %label39
    label33:                                                ; preds = %label_entry
    %op34 = load i32, i32* @opb
    %op35 = load i32, i32* @opc
    %op36 = icmp slt i32 %op34, %op35
    %op37 = zext i1 %op36 to i32
    %op38 = icmp ne i32 %op37, 0
    br i1 %op38, label %label40, label %label46
    label39:                                                ; preds = %label_entry, %label46
  ret i32 0
    label40:                                                ; preds = %label33
    %op41 = load i32, i32* @opc
    %op42 = load i32, i32* @opd
    %op43 = icmp slt i32 %op41, %op42
    %op44 = zext i1 %op43 to i32
    %op45 = icmp ne i32 %op44, 0
    br i1 %op45, label %label47, label %label49
    label46:                                                ; preds = %label33, %label49
    br label %label39
    label47:                                                ; preds = %label40
    %op48 = load i32, i32* @opd
    ret i32 %op48
    label49:                                                ; preds = %label40
    br label %label46   
    }
    define void @main() {
    label_entry:
    br label %label1
    label1:                                                ; preds = %label_entry, %label6
    %op15 = phi i32 [ 0, %label_entry ], [ %op9, %label6 ]
    %op3 = icmp slt i32 %op15, 200000000
    %op4 = zext i1 %op3 to i32
    %op5 = icmp ne i32 %op4, 0
    br i1 %op5, label %label6, label %label10
    label6:                                                ; preds = %label1
    %op7 = call i32 @max()
    %op9 = add i32 %op15, 1
    br label %label1
    label10:                                                ; preds = %label1
    %op11 = load i32, i32* @opa
    call void @output(i32 %op11)
    %op12 = load i32, i32* @opb
    call void @output(i32 %op12)
    %op13 = load i32, i32* @opc
    call void @output(i32 %op13)
    %op14 = load i32, i32* @opd
    call void @output(i32 %op14)
    ret void
    }

    ```
    </details>

    <details><summary>开启常量传播优化后</summary>

    ```llvm
    ; ModuleID = 'cminus'
    source_filename = "ConstPropagation/testcase-3.cminus"

    @opa = global i32 zeroinitializer
    @opb = global i32 zeroinitializer
    @opc = global i32 zeroinitializer
    @opd = global i32 zeroinitializer
    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define i32 @max() {
    label_entry:
    store i32 0, i32* @opa
    store i32 40320, i32* @opb
    store i32 362880, i32* @opc
    store i32 1814400, i32* @opd
    br label %label33
    label33:                                                ; preds = %label_entry%label_entry
    %op34 = load i32, i32* @opb
    %op35 = load i32, i32* @opc
    %op36 = icmp slt i32 %op34, %op35
    %op37 = zext i1 %op36 to i32
    %op38 = icmp ne i32 %op37, 0
    br i1 %op38, label %label40, label %label46
    label39:                                                ; preds = %label46
    ret i32 0
    label40:                                                ; preds = %label33
    %op41 = load i32, i32* @opc
    %op42 = load i32, i32* @opd
    %op43 = icmp slt i32 %op41, %op42
    %op44 = zext i1 %op43 to i32
    %op45 = icmp ne i32 %op44, 0
    br i1 %op45, label %label47, label %label49
    label46:                                                ; preds = %label33, %label49
    br label %label39
    label47:                                                ; preds = %label40
    %op48 = load i32, i32* @opd
    ret i32 %op48
    label49:                                                ; preds = %label40
    br label %label46
    }
    define void @main() {
    label_entry:
    br label %label1
    label1:                                                ; preds = %label_entry, %label6
    %op15 = phi i32 [ 0, %label_entry ], [ %op9, %label6 ]
    %op3 = icmp slt i32 %op15, 200000000
    %op4 = zext i1 %op3 to i32
    %op5 = icmp ne i32 %op4, 0
    br i1 %op5, label %label6, label %label10
    label6:                                                ; preds = %label1
    %op7 = call i32 @max()
    %op9 = add i32 %op15, 1
    br label %label1
    label10:                                                ; preds = %label1
    %op11 = load i32, i32* @opa
    call void @output(i32 %op11)
    %op12 = load i32, i32* @opb
    call void @output(i32 %op12)
    %op13 = load i32, i32* @opc
    call void @output(i32 %op13)
    %op14 = load i32, i32* @opd
    call void @output(i32 %op14)
    ret void
    }

    ```
    </details>

    $\quad$可以看到，优化主要体现在`max()`函数中，优化后的代码替换了块内的常量引用，并将`max()`函数中的第一个条件跳转指令变换为强制跳转指令(由于助教不要求全局变量的块间常量传播，所以第2、3个`if-else`指令并未优化)。

* $\textbf{循环不变式外提}$
    $\textbf{(1)整体框架}$：本优化pass主要分为三个部分(如下面代码所示)：寻找循环不变式```findInv(loop)```，循环不变式外提```hoistInv(prebb)```，以及调用这些函数的主体函数```run()```。接下来将分别讲解每个部分的设计与实现。
    <details><summary>整体框架</summary>

    ```cpp
        std::map<BasicBlock* , std::set<Instruction *>> invariant;
        std::map<BBset_t *,int> hoistRecord;
        std::map<BBset_t *,int> analyRecord;
        void LoopInvHoist::findInv(BBset_t * loop){...}
        void LoopInvHoist::hoistInv(BasicBlock *prebb){...}
        void LoopInvHoist::run(){...}
    ```
    </details>

    $\textbf{(2)findInv(BBset_t * loop)}$: 首先，我们认为，```call(),br(),ret(),alloca(),cmp(),phi()```指令都不是循环不变量，因此我们当我们分析到它们时会选择忽略。随后，我们认为一个循环不变量中出现的变量不应该与非循环不变量中的变量产生数据依赖，即循环不变量对应的指令与非循环不变量对应指令不应该有变量的交集。(由于这是SSA的实现，所以指令可以与左值绑定，具体到代码的实现就是被判断指令的右值与待比较指令的左值不一致)因此，我们只需要如下代码进行简单的遍历判断就可以判断出当前指令是不是循环不变量。

    ```cpp
        for(auto val : ins->get_operands()){
                    if(NoInvariantSet.find(val) != NoInvariantSet.end()){
                        isInv = false;
                    }
                }//若存在数据的依赖关系，则目前分析中不是循环不变式。
    ```
    $\quad$同时，如果表达式是由循环不变量组成的，则这个表达式也是循环不变量。这种性质为我们接下来的处理提供了不少便利:我们首先将```loop```中所有的指令假定为非循环不变量，并放入```NoInvariantSet```中。随后对```loop```中所有指令进行循环不变量判断的操作，找到循环不变量后记录并将其从```NoInvariantSet```删去，这样，最近一个引用该循环不变量的表达式在进行判断时也会被分析为循环不变式。(例如```b=a;c=b;d=c```当```b=a```从```NoInvariantSet```删去时，```c=b```对```b=a```的数据依赖也会消失，在判断时仍然会被认定为循环不变量。)
    $\quad$ 但在```loop```中，指令并不是顺序执行的，其中存在着循环执行，当```BB1:b=a+c; BB2:c=b; BB3:d=c+a;其中a为循环不变量，数据来自循环外，BB块间并不顺序执行```这种情况出现时，先前的单趟处理可能会出现错误的判断。因此要在先前的遍历外层再套一层循环，循环往复的迭代直至```NoInvariantSet```不在变化为止，这样子才能寻找出```loop```中所有的循环不变量。
    <details><summary>$findInv()$的主体</summary>

    ```cpp
    do{
        NeedReScan = false;
        for(auto bb : *loop){
            ins_set.clear();
            for(auto ins : bb->get_instructions()){
                bool isInv = true;
                /**这里略去了跳过特定类型指令分析的代码**/
                for(auto val : ins->get_operands()){
                    if(NoInvariantSet.find(val) != NoInvariantSet.end()){
                        isInv = false;
                    }
                }//若存在数据的依赖关系，则目前分析中不是循环不变式。
                if(isInv){
                    NoInvariantSet.erase(ins);
                    ins_set.insert(ins);
                    NeedReScan = true;
                }
            }
            invariant.insert({bb,ins_set});
        }
    }while(NeedReScan);
    ```
    </details>

    $\quad$当然，上述的解决方法并不是最好的，多次的循环迭代中会有很多无意义的操作，更为优化的做法应该是在第一趟分析时记录整个```loop```中的数据直接源自循环外部的循环不变量，第二趟(或者在第一趟记录的同时做)分析得到整个循环中与第一趟分析得到的循环不变量相关的数据依赖链表，然后对数据依赖链表进行分析，将所有可传递的循环不变式一次全部找出。但是考虑到这种优化并不会使得优化后的目标代码更优，只是减少了优化目标代码时迭代的次数，并不影响实验分数，所以为了减少BUG的产生，我们组还是选择了这种朴素的循环判断。
    $\textbf{(3)hoistInv(BasicBlock *prebb)}$ 这个函数的参数是循环不变量的待提块。而所有的代提式全存于```std::map<BasicBlock* , std::set<Instruction *>> invariant```中，由于要在原```BB```块中删除该指令，我们选择由一个```BB->INS_SET```的map来维护这些信息。外提的思想十分朴素，将先前找到的循环不变量指令按顺序的插入到待提块```prebb```的倒数第二条指令与倒数第一条指令间即可。具体实现便是先记录并删去终止指令，随后插入循环不变量指令，最后将先前记录的终止指令插回指令链尾端即可。
    <details><summary>$hoistInv()$的主体</summary>

    ```cpp
    auto terminatorInstr = prebb->get_terminator();
    auto tmpInstr = terminatorInstr;
    prebb->delete_instr(terminatorInstr);

    for(auto iter = invariant.begin();iter != invariant.end();iter++){
        auto bb = iter->first;
        auto instrSet = iter->second;
        for(auto instr : instrSet){
            prebb->add_instruction(instr);
            bb->delete_instr(instr);
        }
    }

    prebb->add_instruction(tmpInstr);
    return;
    ```
    </details>

    $\textbf{(4)run()}$ 这部分主要就是对函数的所有循环进行不变式寻找与外提，属于相对简单的操作，最开始我们的想法是根据```get_parent_loop(loop)```函数为所有的```loop```构造关系，最后将得到函数的```循环嵌套关系森林forest```，随后对该森林进行```TopoSort```,最后根据拓扑序关系依次对每个```loop```进行```寻找循环不变量；寻找待提块；不变式外提```操作。但是由于一直出现奇怪的BUG，同时时间有限，最终我们决定采用朴素的方法，对每个```loop```进行上述操作，随后访问其父循环并进行相同操作，直至其到最外层循环为止。于此同时，我们还维护两个全局变量```std::map<BBset_t *,int> hoistRecord;std::map<BBset_t *,int> analyRecord;```来尽可能减少不必要的分析。(当然，无论哪种实现方式，都不影响最终优化后的目标代码)
    
    <details><summary>$run()$的主体</summary>

    ```cpp
    bool isHoist = false;
    for(auto func : m_->get_functions()){
        std::unordered_set<BBset_t *> loops_in_func = loop_searcher.get_loops_in_func(func);
        for(auto loop : loops_in_func){
            do{
                invariant.clear();
                isHoist = false;
                if(analyRecord[loop] == 1 && hoistRecord[loop] == 0) break;
                findInv(loop);
                if(!invariant.empty()){
                    auto base = loop_searcher.get_loop_base(loop);
                    for(auto prebb: base->get_pre_basic_blocks()){
                        if(loop_searcher.get_inner_loop(prebb) != loop)
                            hoistInv(prebb);
                    }
                    isHoist = true;
                    hoistRecord.insert({loop,0});
                    analyRecord.insert({loop,1});
                }
                loop = loop_searcher.get_parent_loop(loop);
                if(loop != nullptr) hoistRecord.insert({loop,isHoist});
            }while(loop);
        }
    }
    ```
    </details>
    $\textbf{(5)优化前后对比}$

    我们以下面的`test.cminus`文件为例子：
    <details><summary>test.cminus</summary>

    ```cpp
    void main(void){
        int i;
        int j;
        int k;
        int m;
        int a;
        i = 1;
        j = 100;
        a = 5;
        while (j<0){
            j = j-1;
            i = a+1;
            k = i+1;
            m = k+1;
        }
        return;
    }
    ```
    </details>
    <details><summary>优化前</summary>
    
    ```llvm
    ; ModuleID = 'cminus'
    source_filename = "OwnTests1.cminus"

    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define void @main() {
    label_entry:
     br label %label5
    label5:                                                ; preds = %label_entry, %label10
    %op20 = phi i32 [ %op18, %label10 ], [ undef, %label_entry ]
    %op21 = phi i32 [ %op16, %label10 ], [ undef, %label_entry ]
    %op22 = phi i32 [ 1, %label_entry ], [ %op14, %label10 ]
    %op23 = phi i32 [ 100, %label_entry ], [ %op12, %label10 ]
    %op7 = icmp slt i32 %op23, 0
    %op8 = zext i1 %op7 to i32
    %op9 = icmp ne i32 %op8, 0
    br i1 %op9, label %label10, label %label19
    label10:                                                ; preds = %label5
    %op12 = sub i32 %op23, 1
    %op14 = add i32 5, 1
    %op16 = add i32 %op14, 1
    %op18 = add i32 %op16, 1
    br label %label5
    label19:                                                ; preds = %label5
    ret void
    }
    ```
    </details>
    <details><summary>优化后</summary>
    
    ```llvm
    ; ModuleID = 'cminus'
    source_filename = "OwnTests.cminus"

    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define void @main() {
    label_entry:
    %op14 = add i32 5, 1
    %op16 = add i32 %op14, 1
    %op18 = add i32 %op16, 1
    br label %label5
    label5:                                                ; preds = %label_entry, %label10
    %op20 = phi i32 [ %op18, %label10 ], [ undef, %label_entry ]
    %op21 = phi i32 [ %op16, %label10 ], [ undef, %label_entry ]
    %op22 = phi i32 [ 1, %label_entry ], [ %op14, %label10 ]
    %op23 = phi i32 [ 100, %label_entry ], [ %op12, %label10 ]
    %op7 = icmp slt i32 %op23, 0
    %op8 = zext i1 %op7 to i32
    %op9 = icmp ne i32 %op8, 0
    br i1 %op9, label %label10, label %label19
    label10:                                                ; preds = %label5
    %op12 = sub i32 %op23, 1
    br label %label5
    label19:                                                ; preds = %label5
    ret void
    }
    ```
    </details>
    
    $\quad$由优化前后的结果我们可以看到，在优化前的`label 10`中，三条有着连续数据依赖的循环不变式(`add`指令)均被外提至`label_entry`处，而非循环不变式(`sub`指令)仍然保留在`label 10`中。这验证了我们Pass的正确性。


* $\textbf{活跃变量分析}$
    $\textbf{实现思路}$：使用`slide12 p56`中的活跃变量分析算法，只是将其中的数据流方程$OUT[B] =\cup_{S\in{succ[B]}}IN[S]$修改为$OUT[B] =\cup_{S\in{succ[B]}}(IN[S]-PHI[B-S])$，其中$PHI[B-S]$为S的`phi`指令中不从B传入的变量集合

    <details><summary>$(IN[S]-PHI[B-S])$的计算</summary>

    ```cpp
    auto succ_tmp_live_in = live_in[succBB];
    for (auto instr : succBB->get_instructions()) {
        if (instr->is_phi()) {
            for (int i = 1; i < instr->get_num_operand(); i+=2) {
                if (instr->get_operand(i) != bb) {
                    if (succ_tmp_live_in.find(instr->get_operand(i - 1)) != succ_tmp_live_in.end()) {
                        succ_tmp_live_in.erase(instr->get_operand(i - 1));
                    }
                }
            }
        }
    }
    ```
    </details>

    $\quad$对于`def`和`use`集合的计算，借助SSA形式的特性，`def`集合即为块中所有指令对应的目标变量集合，而块中所有指令的操作数变量集合除去`def`集合即为`use`集合。

    <details><summary>$def和use集合的计算$</summary>

    ```cpp
    void ActiveVars::get_def_vars(Function *fun)
    {
        for (auto bb : fun->get_basic_blocks()) {
            def_vars.insert({bb, {}});
            for (auto instr : bb->get_instructions()) {
                if (instr->is_ret() || instr->is_store()) {
                    continue;
                }
                def_vars[bb].insert(instr);
            }
        }
        return ;
    }

    void ActiveVars::get_use_vars(Function *fun)
    {
        for (auto bb : fun->get_basic_blocks()) {
            use_vars.insert({bb, {}});
            for (auto instr : bb->get_instructions()) {
                if (instr->is_alloca()) {
                    continue;
                }
                else if (instr->is_br()) {
                    if (instr->get_num_operand() == 3) { // 条件跳转
                        auto operand = instr->get_operand(0);
                        if (cast_constantint(operand) || cast_constantfp(operand)) {
                        continue;
                        }
                    else {
                            use_vars[bb].insert(operand);
                        }
                    }
                }
                else if (instr->is_phi()) {
                    for (int i = 0; i < instr->get_num_operand(); i+=2) {
                    auto operand = instr->get_operand(i);
                        if (cast_constantint(operand) || cast_constantfp(operand)) {
                            continue;
                        }
                        else {
                        use_vars[bb].insert(operand);
                        }
                    }
                }
                else {
                    for (auto operand : instr->get_operands()) {
                        if (instr->is_call()) {
                            if (operand == instr->get_operand(0)) { // 函数名
                                continue;
                            }
                        }
                        if (cast_constantint(operand) || cast_constantfp(operand)) {
                            continue;
                        }
                        else {
                            use_vars[bb].insert(operand);
                        }
                    }
                }
            }
            for (auto var : def_vars[bb]) {
                if (use_vars[bb].find(var) != use_vars[bb].end()) {
                    use_vars[bb].erase(var);
                }
            }
        }
        return ;
    }
    ```
    </details>

    $\quad$由于每次迭代后，得到的`IN`和`OUT`集合都不小于迭代前的`IN`和`OUT`集合(归纳法可证)，因此`IN`和`OUT`集合的更新更加方便，只需将当前迭代出的`IN`和`OUT`集合中不在迭代前对应集合中的元素插入对应集合即可。若迭代后的`IN`集合有元素不在迭代前的`IN`集合中，则说明需要再次迭代。若所有迭代后的`IN`集合都不大于迭代前的`IN`集合，则已得到最终解。

    <details><summary>$迭代过程$</summary>

    ```cpp
    for (auto var : tmp_live_out) { // 迭代后的in和out必不可能小于迭代前的in和out(归纳法可证)
        if (live_out[bb].find(var) == live_out[bb].end()) {
            live_out[bb].insert(var);
        }
    }
    auto tmp_live_in = tmp_live_out;
    for (auto def_var : def_vars[bb]) {
        if (tmp_live_in.find(def_var) != tmp_live_in.end()) {
            tmp_live_in.erase(def_var);
        }
    }
    for (auto use_var : use_vars[bb]) {
        if (tmp_live_in.find(use_var) == tmp_live_in.end()) {
            tmp_live_in.insert(use_var);
        }
    }
    for (auto var : tmp_live_in) {
        if (live_in[bb].find(var) == live_in[bb].end()) {
            live_in[bb].insert(var);
            repeat = true;
        }
    }
    ```
    </details>

### 实验总结

本次实验让我们对独立于机器的IR优化这一章节有了更深刻的理解，尤其是对于活跃变量相关的知识点有更全面的认识。除此之外，还有对`phi`节点的定义，计算，以及对优化分析时的影响有了充分的认知。

### 实验反馈 （可选 不会评分）

感谢助教为我们提供了丰富的接口，以及`phase1`的PASS，让我们在实验过程中节省了不少探索的时间，同时感谢助教降低了要求，省去了很多繁琐情况的考虑(如数组，全局等)。

### 组间交流 （可选）

