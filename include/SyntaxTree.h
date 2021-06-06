#ifndef _MHSJ_SYNTAX_TREE_H_
#define _MHSJ_SYNTAX_TREE_H_

#include <vector>
#include <memory>
#include <string>

#include "../build/location.hh"

namespace SyntaxTree
{
// Use unique postd::stringtype to reference stored objects
template <typename T>
using Ptr = std::shared_ptr<T>;

// List of reference of type
template <typename T>
using PtrList = std::vector<Ptr<T>>;

using Position = yy::location;

// Enumerations
enum class Type
{
    INT = 0,
    VOID
};

enum class BinOp
{
    PLUS = 0,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO
};

enum class UnaryOp
{
    PLUS = 0,
    MINUS
};

enum class UnaryCondOp
{
    NOT = 0
};

enum class BinaryCondOp
{
    LT = 0,
    LTE,
    GT,
    GTE,
    EQ,
    NEQ,
    LAND,
    LOR
};

// Forward declaration
struct Node;
struct Assembly;
struct GlobalDef;
struct FuncDef;

struct Expr;
struct BinaryExpr;
struct UnaryExpr;
struct LVal;
struct Literal;

struct Stmt;
struct VarDef;
struct AssignStmt;
struct FuncCallStmt;
struct ReturnStmt;
struct BlockStmt;
struct EmptyStmt;

struct FuncParam;
struct FuncFParamList;
struct UnaryCondExpr;
struct BinaryCondExpr;

struct IfStmt;
struct WhileStmt;
struct BreakStmt;
struct ContinueStmt;
struct InitVal;

struct Visitor;

// Virtual base of all kinds of syntax tree nodes.
struct Node
{
    Position loc;
    // Used in Visitor. Irrelevant to syntax tree generation.
    virtual void accept(Visitor &visitor) = 0;
};

//node for initial value
struct InitVal: Node{
    bool isExp;
    std::vector<Ptr<InitVal>> elementList;
    Ptr<Expr> expr;
    virtual void accept(Visitor &visitor) override final;
};

// Root node of an ordinary syntax tree.
struct Assembly : Node
{
    PtrList<GlobalDef> global_defs;
    virtual void accept(Visitor &visitor) override final;
};

// Virtual base of global definitions, function or variable one.
struct GlobalDef : virtual Node
{
    virtual void accept(Visitor &visitor) override = 0;
};

// Function definition.
struct FuncDef : GlobalDef
{
    Type ret_type;
    Ptr<FuncFParamList> param_list;
    std::string name;
    Ptr<BlockStmt> body;
    virtual void accept(Visitor &visitor) override final;
};

// Virtual base for statements.
struct Stmt : virtual Node
{
    virtual void accept(Visitor &visitor) = 0;
};

// Variable definition. Multiple of this would be both a statement and a global definition; however, itself only
// represents a single variable definition.
struct VarDef : Stmt, GlobalDef
{
    bool is_constant;
    Type btype;
    std::string name;
    bool is_inited; // This is used to verify `{}`
    PtrList<Expr> array_length; // empty for non-array variables
    Ptr<InitVal> initializers;
    virtual void accept(Visitor &visitor) override final;
};

// Assignment statement.
struct AssignStmt : Stmt
{
    Ptr<LVal> target;
    Ptr<Expr> value;
    virtual void accept(Visitor &visitor) override final;
};

// Return statement.
struct ReturnStmt : Stmt
{
    Ptr<Expr> ret;  // nullptr for void return
    virtual void accept(Visitor &visitor) override final;
};

// BlockStmt statement.
struct BlockStmt : Stmt
{
    PtrList<Stmt> body;
    virtual void accept(Visitor &visitor) override final;
};

// Empty statement (aka a single ';').
struct EmptyStmt : Stmt
{
    virtual void accept(Visitor &visitor) override final;
};

struct ExprStmt : Stmt
{
    Ptr<Expr> exp;
    virtual void accept(Visitor &visitor) override final;
};

// Virtual base of expressions.
struct Expr : Node
{
    virtual void accept(Visitor &visitor) = 0;
};

struct CondExpr : Expr
{
    virtual void accept(Visitor &visitor) = 0;
};

struct AddExpr : Expr
{
    virtual void accept(Visitor &visitor) = 0;
};


struct UnaryCondExpr : CondExpr{//FIXME:FINISH THIS
    UnaryCondOp op;
    Ptr<Expr> rhs;
    virtual void accept(Visitor &visitor) override final;
};

struct BinaryCondExpr : CondExpr{
    BinaryCondOp op;
    Ptr<Expr> lhs,rhs;
    virtual void accept(Visitor &visitor) override final;
};

// Expression like `lhs op rhs`.
struct BinaryExpr : AddExpr
{
    BinOp op;
    Ptr<Expr> lhs, rhs;
    virtual void accept(Visitor &visitor) override final;
};

// Expression like `op rhs`.
struct UnaryExpr : AddExpr
{
    UnaryOp op;
    Ptr<Expr> rhs;
    virtual void accept(Visitor &visitor) override final;
};

// Expression like `ident` or `ident[exp]`.
struct LVal : AddExpr
{
    std::string name;
    PtrList<Expr> array_index; // nullptr if not indexed as array
    virtual void accept(Visitor &visitor) override final;
};

// Expression constructed by a literal number.
struct Literal : AddExpr
{
    bool is_int;
    int int_const;
    //double float_const;
    virtual void accept(Visitor &visitor) override final;
};

// Function call statement.
struct FuncCallStmt : AddExpr
{
    std::string name;
    PtrList<Expr> params;
    virtual void accept(Visitor &visitor) override final;
};

struct FuncParam : Node
{
    std::string name;
    Type param_type;
    PtrList<Expr> array_index; // nullptr if not indexed as array
    virtual void accept(Visitor &visitor) override final;
};

struct FuncFParamList : Node
{
    PtrList<FuncParam> params;
    virtual void accept(Visitor &visitor) override final;
};

struct IfStmt : Stmt
{
    Ptr<Expr> cond_exp;
    Ptr<Stmt> if_statement;
    Ptr<Stmt> else_statement;
    virtual void accept(Visitor &visitor) override final;
};

struct WhileStmt : Stmt
{
    Ptr<Expr> cond_exp;
    Ptr<Stmt> statement;
    virtual void accept(Visitor &visitor) override final;
};

struct BreakStmt : Stmt
{
    virtual void accept(Visitor &visitor) override final;
};

struct ContinueStmt : Stmt
{
    virtual void accept(Visitor &visitor) override final;
};

// Visitor base type
class Visitor
{
public:
    virtual void visit(Assembly &node) = 0;
    virtual void visit(FuncDef &node) = 0;
    virtual void visit(BinaryExpr &node) = 0;
    virtual void visit(UnaryExpr &node) = 0;
    virtual void visit(LVal &node) = 0;
    virtual void visit(Literal &node) = 0;
    virtual void visit(ReturnStmt &node) = 0;
    virtual void visit(VarDef &node) = 0;
    virtual void visit(AssignStmt &node) = 0;
    virtual void visit(FuncCallStmt &node) = 0;
    virtual void visit(BlockStmt &node) = 0;
    virtual void visit(EmptyStmt &node) = 0;
    virtual void visit(ExprStmt &node) = 0;
    virtual void visit(FuncParam &node) = 0;
    virtual void visit(FuncFParamList &node) = 0;
    virtual void visit(IfStmt &node) = 0;
    virtual void visit(WhileStmt &node) = 0;
    virtual void visit(BreakStmt &node) = 0;
    virtual void visit(ContinueStmt &node) = 0;
    virtual void visit(UnaryCondExpr &node) = 0;
    virtual void visit(BinaryCondExpr &node) = 0;
    virtual void visit(InitVal &node) = 0;
};
} // end namespace SyntaxTree

#endif // _MHSJ_SYNTAX_TREE_H_
