# Lab4 实验报告

曾源   PB18111741
黄庄湫 PB18111719

## 实验要求

本实验的主要工作是：在给定的语义规则和框架下完成cminusf_builder.cpp,使之通过访问者模式对实验框架生成的AST进行相应操作，生成中间代码(ll文件)。

## 实验难点

* 对变量左值与变量右值的判断与处理，且要对两种表达式(assign & simple)进行区分。（var的处理）
* 对于无条件跳转与返回指令的逻辑设计(后续简称为br-ret问题)。
* 函数形参列表的获取。

## 实验设计

1. &emsp; 整体框架设计与全局变量维护
   * $\textbf{整体框架设计}$ &emsp;根据打印AST的算法来确定每个结点的访问顺序，并且在访问结点时进行相应操作,维护全局信息并生成ll指令。而具体操作则根据语义规则实现，包括但不限于类型装换，无条件跳转，生成load与store指令等。这些将在后续的全局变量设计与难点解决处详细展开。
   
   * $\textbf{全局变量维护}$ &emsp;尽可能少使用全局变量，只在必要时维护全局信息.最终保留的全局变量包括：全局的形参列表，Address，Expression Value，以及后续为了解决br-ret问题以及左值右值问题而维护的全局信息等等。具体如下表：
    $\begin{array}{l|l}
    \hline
    全局变量 & 作用 \\
    \hline
    Params & 存储FunDeclaration中的node.params \\ & 用于在CompoundStmt的中为形参开辟存储空间并传递实参 \\
    \hline
    ParamsType & 存储形参的类型列表 \\
    \hline
    Address & 指向当前赋值语句左值的地址 \\
    \hline
    ExpressionValue & 指向当前计算出的表达式的值 \\
    \hline
    retAlloca & 指向当前函数返回值的地址 \\
    \hline
    retBB & 为每个函数都保留的,专门负责return语句的基本块 \\
    \hline
    is\_left\_var\_assign & 表明分析到当前var时，该变量是否为赋值语句的左值 \\
    \hline
    is\_var\_assign & 表明分析到当前var时，该变量是否为赋值语句的组成部分 \\
    \hline
    have\_return & 表明当前的基本块中是否含有return语句 \\ & (用于if-else语句和while语句循环体结束后跳转方向的判断) \\ & 若有,则最后跳转到retBB,否则跳转到nextBB \\
    \hline
    \end{array}$
2. &emsp;难点以及解决方案
   * $\textbf{var的处理}$ 
     * 最开始的处理中我们只是简单的在var处进行Load和将var的地址记录在全局变量中，这就导致在分析赋值语句时，全局的地址是会改变为右值的地址(通过解读生成的ll文件发现)，进而导致生成的store指令的地址参数出错。为了解决这一问题，我们先是设计了一个$is\_left$来区分左右值，进而进行特判：左值存地址，右值载入值。后续发现当var由simple_expression展开得来时，没有左右值之分，于是便将$is\_left$这一全局信息改为$is\_var\_assign$与$is\_left\_var\_assign$来进行特判。具体的特判如下：
      ```cpp
      if(is_left_var_assign == true && is_var_assign ==true){
         Address = tmpAddress;
      }
      if ((is_left_var_assign == false && is_var_assign == true) || is_var_assign == false) {
      ExpressionValue = builder->create_load(tmpAddress);
      }
      ```
   * $\textbf{br-ret问题的处理}$ 
     * 测试样例时出现的第一个大问题是在if-else语句和while语句的循环体中出现return语句时,虽然能生成ll文件,但最终执行时却会出现段错误,究其原因,是处理return语句的逻辑有问题.
     * 我们最初的设计是将ret语句的创建放在ReturnStmt中,并将retBB的创建和插入放在SelectionStmt和IterationStmt语句中(此时retBB为局部变量),所以若if-else语句和while语句的循环体中出现return语句,则在无条件跳转到nextBB的语句之前会有ret语句,这会导致匿名块的产生并最终导致实际执行时出现错误.若函数的最后一个部分为if-else语句,并且该语句中所有的循环体都有return语句,如样例12,则最后的nextBB块为空块,此时执行ll文件时也会出现错误.
     * 在第一版的方案中,为了避免出现空块的情况,我们选择在每个nextBB的开头创建一句冗余的load语句,但由于无法解决匿名块的问题,我们最终抛弃了这一想法.
     * 我们最终的解决方案基于以下假定[(issue#205)](http://222.195.68.197/staff/2020-fall-notice_board/-/issues/205):对于函数的每一条可能的执行路径,有且仅有一条ret语句并且该ret语句必定为路径上最后一条语句.
     * 我们将ret语句的创建与retBB的创建和插入都放在了FunDeclaration中,确保每一个函数有且仅有一个ret语句,同时另设一全局变量have_return来表示当前基本块中是否含有return语句,若有,则跳转到retBB,否则跳转到nextBB,将函数其他部分的return语句全部以无条件跳转到retBB来实现,这样就解决了一个基本块中同时有不止一条无条件跳转或ret语句的问题.同时,由于函数最后必定为ret语句,所以不可能出现空块的问题.
   * $\textbf{获取函数形参列表的处理}$ 
     * 参考了[issue#201](http://222.195.68.197/staff/2020-fall-notice_board/-/issues/201),并使用is_function_type()进行转换的合法性检查。
3. &emsp;类型转换
   * 这一步并不复杂，只要在需要转换的地方进行判断并生成转换指令即可,只是单纯的重复工作。
   * 具体需要类型转换的地方为:赋值语句，simple_expression中的各种操作，数组偏移量的计算，以及对于函数实参、返回值的转换。

### 实验总结

通过本次实验,我们锻炼了通过git进行协同工作的能力，进一步了解了访问者模式，加深了对cminusf语法和中间代码生成逻辑的理解。同时，在debug过程中，我们也更加深刻的理解到了对全局变量进行谨慎操作的重要性(包括但不限于全局指针的修改与释放)，对于面向对象编程也有了一定程度的理解。

### 实验反馈

本次实验整体难度适中,尤其是在助教提供了打印抽象语法树的算法来帮助理解访问者模式下算法的执行流程后,整体难度大幅下降.助教提供的12个样例中,只有最后一个考察了较复杂的情况,希望助教能够提供更多复杂的样例以供测试,帮助同学们提高实验完成度,加深对cminusf语法和中间代码生成逻辑的理解.

### 组间交流 

组长:刘扬 学号:PB18051093
关于访问ASTProgram结点时遇到的作用域问题(会导致全局变量无法定义并初始化),蒋易志同学(PB18111754)指出,在[cminusf_builder.hpp](../../include/cminusf_builder.hpp)中已经进入了一次作用域,无需再enter一次.
