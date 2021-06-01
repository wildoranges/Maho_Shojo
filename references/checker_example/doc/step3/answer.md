# task3
```
在doc/step3/answer.md里面介绍你的具体设计以及核心代码实现、实验中遇到的困难以及解决方案、对实验的吐槽以及建议。
```
## 设计实现
`SynTaxTreeChecker.h`中的部分主要修改:
`SyntaxTreeChecker`类中加入成员变量`haserror`用于指示是否有错误。遇到错误时该域置位true。`ExprType`用于当访问Expr类节点时指示其类型。`FuncType`用于指示正在访问的函数类型。`hasRet`用于指示函数是否有返回语句。`isConst`用于指示Expr是否是常量表达式。`INTConstVal`指示整形常量表达式的值。`FLTConstVal`用于指示浮点型常量表达式的值。
新定义类`Variable`和其子类`ConstVar`分别表示非常量/常量变量。域中有isused。变量被引用时被设置为true。变量`function`为函数。`PtrFunction`为指向Function的指针。`declare_variable`通过is_const域判断应该构造哪个类。`functions`为`std::unordered_map<std::string,PtrFunction>`，用于存储已经声明的函数。`declare_function`用于检查是否重名并声明函数。exit_scope()时检查此作用域变量是否被使用。若未被使用，发出警告。
`SynTaxTreeChecker.cpp`及其他的部分主要修改:
在遇到错误时，不直接exit。根据信息选择跳过或其他处理。若信息不全，则跳过该错误(返回)。若可以确定部分信息(比如在Expr类型表达式中虽然发现错误但可以定型时)则更新这部分信息后返回。最后在全部检查结束后(main.cpp)判断是否发现了错误(haserror域)。若是true，则exit(1)
`Assembly`处理时，在exit_scope前检查main函数的相关信息若没有main函数或着返回值不是int则报错。
`FuncDef`处理时，若重定义，则报错。接着设置FuncType为函数的返回类型，并检查函数的body。检查结束后若没有`return`且函数类型不为空且不是main函数，则警告。
`VarDef`处理时首先判断是否重定义。若是，则报错。若类型是VOID,报错。接着判断，若是数组类型，则检查array_length中每一个元素。若不是常量表达式或者不是INT或者小于0，则报错。若是，则每次更新数组长总度(arrlen *= INTConstVal)以及每一维定义的长度(parray->push_back(INTConstVal))；之后对initalizers中的表达式进行检查。表达式个数大于数组长度则报错。若类型是VOID，则报错。若有类型转换，则警告。若不是数组类型。判断initializes长度是否大于1，若是则报错。对initializers中的expr进行检查，若不是const且变量是const，则报错。若是VOID,则报错。若有类型转换，则警告。最后调用declare_variable()
`AssignStmt`处理时，判断左值是否常量，若是则报错。之后判断右值若为VOID类型，则报错。若有类型转换，则警告。
`BlockStmt`处理时先enter_scope,之后对其中每一个expr进行检查。最后exit_scope。
`EmptyStmt`直接返回
`ExprStmt`对其中的Expr进行检查。
在对Expr类型的节点(如:Lval,FunCallStmt等)进行访问时，根据子表达式（递归)的信息，来确定当前表达式的信息(类型，是否是常量，若是常量，值是多少...)或者输出警告、错误。
在`BinaryExpr`中。首先递归的对lhs,rhs使用accept()，获取左右表达式的相关信息。若有VOID类型，则报错。接着根据左右表达式的类型，并结合`op`的类型来确定是否有错误(除0，%不能有float类型的操作数等)、警告以及当前表达式的信息。
`UnaryExpr处理方式和上面类似`
`Lval`处理时。首先判断变量是否被定义。若未被定义，则报错。
```c++
auto Valptr = lookup_variable(node.name);
if(Valptr==nullptr)
{
    ...
}
```
若找到该变量，则设其isused为true。
接着判断该变量是否为常量。若非常量。判断此时的索引个数和定义的维数是否相等。不相等则报错。之后对ValPtr中的array_length进行遍历。同时对array_index中的值进行遍历。若array_index非INT或者小于0,则报错。若是非负的INT且是常量，和array_length当前遍历的值进行比较判断是否越界。
若是常量，判断索引个数是否和定义维数匹配。否则报错。接着对initalizers中的expr进行遍历来获取此常量的值。最后更新ExprType等信息。
`Literal`处理时设置isConst为true并根据类型来更新ExprType，INTConstVal，FLTConstVal等信息。
`FunCallStmt`处理时判断是否已经定义，若是则报错。接着根据函数类型设置ExprType等信息。

## 实验遇到的困难及解决方案
尝试多次，样例6均不过。后来发现是GCC版本问题(本机版本是10.2.0)。GCC对部分错误/警告的处理与本实验要求不一致（比如关于函数返回值的处理）。一开始时按照GCC的错误、警告方式处理，导致样例6错误。按照要求更改后样例通过。

## 吐槽及建议
若样例6不能公开，可以多给点测试集用于测试程序。公开的测试样例有点少导致DEBUG时像无头苍蝇一样乱撞。


