#ifndef _C1_SYNTAX_TREE_H_
#define _C1_SYNTAX_TREE_H_

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
    FLOAT,
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

struct Visitor;

// Virtual base of all kinds of syntax tree nodes.
struct Node
{
    Position loc;
    // Used in Visitor. Irrelevant to syntax tree generation.
    virtual void accept(Visitor &visitor) = 0;
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
    PtrList<Expr> initializers;
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

// Expression like `lhs op rhs`.
struct BinaryExpr : Expr
{
    BinOp op;
    Ptr<Expr> lhs, rhs;
    virtual void accept(Visitor &visitor) override final;
};

// Expression like `op rhs`.
struct UnaryExpr : Expr
{
    UnaryOp op;
    Ptr<Expr> rhs;
    virtual void accept(Visitor &visitor) override final;
};

// Expression like `ident` or `ident[exp]`.
struct LVal : Expr
{
    std::string name;
    PtrList<Expr> array_index; // nullptr if not indexed as array
    virtual void accept(Visitor &visitor) override final;
};

// Expression constructed by a literal number.
struct Literal : Expr
{
    bool is_int;
    int int_const;
    double float_const;
    virtual void accept(Visitor &visitor) override final;
};

// Function call statement.
struct FuncCallStmt : Expr
{
    std::string name;
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
};
} // end namespace SyntaxTree

#endif // _C1_SYNTAX_TREE_H_
