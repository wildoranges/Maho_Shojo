#ifndef _C1_SYNTAX_TREE_PRINTER_H_
#define _C1_SYNTAX_TREE_PRINTER_H_

#include "SyntaxTree.h"

class SyntaxTreePrinter : public SyntaxTree::Visitor
{
public:
    virtual void visit(SyntaxTree::Assembly &node) override;
    virtual void visit(SyntaxTree::FuncDef &node) override;
    virtual void visit(SyntaxTree::BinaryExpr &node) override;
    virtual void visit(SyntaxTree::UnaryExpr &node) override;
    virtual void visit(SyntaxTree::LVal &node) override;
    virtual void visit(SyntaxTree::Literal &node) override;
    virtual void visit(SyntaxTree::ReturnStmt &node) override;
    virtual void visit(SyntaxTree::VarDef &node) override;
    virtual void visit(SyntaxTree::AssignStmt &node) override;
    virtual void visit(SyntaxTree::FuncCallStmt &node) override;
    virtual void visit(SyntaxTree::BlockStmt &node) override;
    virtual void visit(SyntaxTree::EmptyStmt &node) override;
    virtual void visit(SyntaxTree::ExprStmt &node) override;
    virtual void visit(SyntaxTree::FuncParam &node) override;
    virtual void visit(SyntaxTree::FuncFParamList &node) override;
    void print_indent();
private:
    int indent = 0;
};

#endif  // _C1_SYNTAX_TREE_PRINTER_H_
