## 1
`SyntaxTree.h`中声明了`Node`类，是所有语法树节点的抽象基类。`Assembly`类是一棵语法树的根节点 `GlobalDef`类是所有定义(函数、变量)的抽象基类。`FuncDef`类表示函数定义。`Expr`是所有表达式的抽象基类。`BinaryExpr`类是二元表达式。`UnaryExpr`是一元表达式。`LVal`是左值表达式。`Literal`是常量(int,float)。`Stmt`是所有statements的抽象基类。`VarDef`是变量定义。`AssignStmt`是赋值语句。`FuncCallStmt`是函数调用。`ReturnStmt`是返回语句。`BlockStmt`是语句块。`EmptyStmt`是空语句。`Visitor`是抽象访问者基类。

`SyntaxTree.cpp`中定义了上述的各个非抽象基类中接受访问者的动作。-让访问者访问自身节点。

## 2
在`C1Driver::parse`中使用了语法分析产生的`yy::C1Parser`来构建语法树同时控制词法分析器的输入流以及语法分析的错误处理。`yy::C1Parser`通过`C1Driver`中指定的lexer词法分析类来进行语法分析，以及`C1Driver`中指定的错误处理进行错误处理。`C1Driver`可以看做是分析器的驱动、控制器。驱动词法，语法分析器的工作。

## 3
- $Exp \to PLUS Exp_2$单操作数表达式，此时为Exp创建UnaryExpr类型节点，并为其中的的操作符赋值为PLUS,右操作数赋值为指向$Exp_2$的指针。最后更新位置信息。
  $Exp \to MINUS Exp_2$单操作数表达式，语法树构建与上类似
  $Exp \to Exp_1 PLUS Exp_2$双操作数表达式。此时为Exp创建BinaryExpr节点，为其中的操作符赋为PLUS，左操作数赋值为指向$Exp_1$的指针，右操作数赋为指向$Exp_2$的指针。最后更新位置信息
  $Exp \to Exp_1 MINUS Exp_2$同上
  $Exp \to Exp_1 MULTIPLY Exp_2$同上
  $Exp \to Exp_1 DIVIDE Exp_2$同上
  $Exp \to Exp_1 MODULO Exp_2$同上
  $Exp \to LPARENTHESE Exp_1 RPARENTHESE$此时Exp的节点就是Exp_1的节点
  $Exp \to Lval$此时Exp节点就是Lval节点
  $Exp \to Number$同上

- $Stmt \to LVal ASSIGN Exp SEMICOLON$赋值语句。为Stmt创建AssignStmt类型节点。并将target赋值为指向Lval的指针，value赋值为指向Exp的指针。最后更新位置信息。
  $Stmt \to IDENTIFIER LPARENTHESE RPARENTHESE SEMICOLON$函数调用。此时为Stmt创建FuncCallStmt节点，并将函数名赋值为该标识符。最后更新位置信息
  $Stmt \to RETURN OptionRet SEMICOLON$返回语句。此时为Stmt创建ReturnStmt节点。并将ret赋值为指向OptionRet的指针。最后更新位置
  $Stmt \to Block$语句块。此时Stmt节点就是Block节点
  $Stmt \to SEMICOLON$空语句。此时为Stmt创建EmptyStmt节点。最后更新位置

- $FuncDef \to DefType IDENTIFIER LPARENTHESE RPARENTHESE Block$函数定义。此时为FuncDef创建FuncDef节点。设置返回值类型为DefType。函数名为标识符IDENTIFIER。函数体为指向Block的指针。最后更新位置

## 4
### 1)
`SyntaxTreePrinter`是`SyntaxTree::Visitor`的子类（具体实现）。语法树节点接受(`accept`)一个`Visitor`类，并调用其定义的`visit`函数来访问该节点

### 2)
- VarDef
  首先打印缩进之后判断是否为常量类型，若是则输出`const `。之后输出该变量的类型和名字。之后继续调用`SyntaxTreePrinter::visit`(legnth->accept(*this)，length也会调用此时的visit函数)输出`[]`中内容并将`is_array`设置为true（若非数组，则是null，不需要输出，`is_array`是false）。之后判断是否有初始化，若有则输出` = `若是数组则输出`{` 并继续递归使用`SyntaxTreePrinter::visit`，输出初始化的内容。最后若是数组则输出`}`。最后输出`;`

- BinaryExpr
  输出`(`，并调用`SyntaxTreePrinter::visit`输出左表达式。接着输出操作符。如何调用`SyntaxTreePrinter::visit`输出右表达式和`)`

- BlockStmt
  首先打印若干缩进。之后输出`{`。并增加缩进4(层次化表示)。之后对block体内每一个的stmt调用`SyntaxTreePrinter::visit`输出其中内容。最后将增加的缩进删去并打印若干缩进。最后输出`}`

- FuncDef
  首先输出函数返回值类型、函数名以及一对括号。之后对函数体调用`SyntaxTreePrinter::visit`输出其中内容。最后将缩进设为0