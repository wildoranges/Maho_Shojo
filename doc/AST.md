# AST

**源码链接：**

[SyntaxTree.h](../include/SyntaxTree.h)

[SyntaxTree.cpp](../src/SyntaxTree.cpp)


## 目录

[Type](#Type)

[Binop](#Binop)

[UnaryOp](#UnaryOp)

[UnaryCondOp](#unarycondop)

[BinaryCondOp](#binarycondop)

[Node](#Node)

## Type
包含SysY语言支持的数据类型：`Type::INT`以及`Type::VOID`。

多出的`Type::STRING`类型用作系统调用的参数类型，`Type::BOOL`作为条件表达式类型。

## Binop

双目算术表达式的操作符。包含

`Binop::PLUS` 加

`Binop::MINUS`减

`Binop::MULTIPLY`乘

`Binop::DIVIDE`除

`Binop::MODULO`模

## UnaryOp

单目算术表达式操作符，包含

`UnaryOp::PLUS`正

`UnaryOp::MINUS`负


## UnaryCondOp

单目条件表达式操作符，包含

`UnaryCondOp::NOT`非

## BinaryCondOp

双目条件表达式操作符，包含

`BinaryCondOp::LT`小于

`BinaryCondOp::LTE`小于等于

`BinaryCondOp::GT`大于

`BinaryCondOp::GTE`大于等于

`BinaryCondOp::EQ`等于等于

`BinaryCondOp::NEQ`不等于

`BinaryCondOp::LAND`逻辑与

`BinaryCondOp::LOR`逻辑或

## Node

语法树所有结点的基类，

`Node::loc`是其在对应源文件的位置信息

`virtual void Node::accept(Visitor &visitor)`为虚函数，用于访问者模式，接受一个[Visitor](#visitor)。需要进行重写

## Assembly

## InitVal

## GlobalDef

## FuncDef

## Vardef

## Stmt

## AssignStmt

## ReturnStmt

## BlockStmt

## EmptyStmt

## ExprStmt

## Expr

## CondExpr

## AddExpr

## UnaryCondExpr

## BinaryCondExpr

## BinaryExpr

## UnaryExpr

## LVal

## Literal

## FuncCallStmt

## FuncParam

## FuncParamList

## IfStmt

## WhileStmt

## BreakStmt

## ContinueStmt

## Visitor