
#include "SyntaxTree.h"

using namespace SyntaxTree;

void Assembly::accept(Visitor &visitor) { visitor.visit(*this); }
void FuncDef::accept(Visitor &visitor) { visitor.visit(*this); }
void BinaryExpr::accept(Visitor &visitor) { visitor.visit(*this); }
void UnaryExpr::accept(Visitor &visitor) { visitor.visit(*this); }
void LVal::accept(Visitor &visitor) { visitor.visit(*this); }
void Literal::accept(Visitor &visitor) { visitor.visit(*this); }
void ReturnStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void VarDef::accept(Visitor &visitor) { visitor.visit(*this); }
void AssignStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void FuncCallStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void BlockStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void EmptyStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void ExprStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void FuncParam::accept(Visitor &visitor) { visitor.visit(*this); }
void FuncFParamList::accept(Visitor &visitor) { visitor.visit(*this); }
void IfStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void WhileStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void BreakStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void ContinueStmt::accept(Visitor &visitor) { visitor.visit(*this); }
void UnaryCondExpr::accept(Visitor &visitor) { visitor.visit(*this); }
void BinaryCondExpr::accept(Visitor &visitor) { visitor.visit(*this); }
void InitVal::accept(Visitor &visitor) { visitor.visit(*this); }
/*
void Stmt::accept(Visitor &visitor) {
    auto var_def = dynamic_cast<VarDef *>(this);
    if (var_def) {
        var_def->accept(visitor);
        return;
    }
    auto if_stmt = dynamic_cast<IfStmt *>(this);
    if (if_stmt) {
        if_stmt->accept(visitor);
        return;
    }
    auto while_stmt = dynamic_cast<WhileStmt *>(this);
    if (while_stmt) {
        while_stmt->accept(visitor);
        return;
    }
    auto break_stmt = dynamic_cast<BreakStmt *>(this);
    if (break_stmt) {
        break_stmt->accept(visitor);
        return;
    }
    auto continue_stmt = dynamic_cast<ContinueStmt *>(this);
    if (continue_stmt) {
        continue_stmt->accept(visitor);
        return;
    }
    auto assign_stmt = dynamic_cast<AssignStmt *>(this);
    if (assign_stmt) {
        assign_stmt->accept(visitor);
        return;
    }
    auto ret_stmt = dynamic_cast<ReturnStmt *>(this);
    if (ret_stmt) {
        ret_stmt->accept(visitor);
        return;
    }
    auto block_stmt = dynamic_cast<BlockStmt *>(this);
    if (block_stmt) {
        block_stmt->accept(visitor);
        return;
    }
    auto empty_stmt = dynamic_cast<EmptyStmt *>(this);
    if (empty_stmt) {
        empty_stmt->accept(visitor);
        return;
    }
    auto expr_stmt = dynamic_cast<ExprStmt *>(this);
    if (expr_stmt) {
        expr_stmt->accept(visitor);
        return;
    }
}

void GlobalDef::accept(Visitor &visitor) {
    auto var_def = dynamic_cast<VarDef *>(this);
    if (var_def) {
        var_def->accept(visitor);
        return;
    }
    auto func_def = dynamic_cast<FuncDef *>(this);
    if (func_def) {
        func_def->accept(visitor);
        return;
    }
}

void Expr::accept(Visitor &visitor) {
    auto cond_expr = dynamic_cast<CondExpr *>(this);
    if (cond_expr) {
        cond_expr->accept(visitor);
        return;
    }
    auto add_expr = dynamic_cast<AddExpr *>(this);
    if (add_expr) {
        add_expr->accept(visitor);
        return;
    }
}

void CondExpr::accept(Visitor &visitor) {
    auto unary_cond_expr = dynamic_cast<UnaryCondExpr *>(this);
    if (unary_cond_expr) {
        unary_cond_expr->accept(visitor);
        return;
    }
    auto binary_cond_expr = dynamic_cast<BinaryCondExpr *>(this);
    if (binary_cond_expr) {
        binary_cond_expr->accept(visitor);
        return;
    }
}*/