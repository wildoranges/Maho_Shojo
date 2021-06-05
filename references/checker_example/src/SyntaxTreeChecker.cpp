#include "SyntaxTreeChecker.h"
#include <system_error>
#include <map>

using namespace SyntaxTree;

void SyntaxTreeChecker::visit(Assembly &node)
{
    enter_scope();
    for (auto def : node.global_defs) {
        def->accept(*this);
    }
    if(!functions.count(std::string("main")))
    {
        haserror = true;
        std::cout<<"error,NoMainFunc,err_code : "<<warn_code["NoMainFunc"]<<std::endl;
    }
    else
    {
        auto Funcname = std::string("main");
        auto FuncP = lookup_function(Funcname);
        if(FuncP->func_def->ret_type !=Type::INT)
        {
            //haserror = true;
            std::cout<<"warning,WMainFunc,err_code: "<<warn_code["WMainFunc"]<<" expect return int\n";
        }
    }
    exit_scope();
}

void SyntaxTreeChecker::visit(FuncParam &node)
{
    return;
}

void SyntaxTreeChecker::visit(FuncFParamList &node)
{
    std::map<std::string,Ptr<FuncParam>> param_map;
    for(auto param:node.params)
    {
        if(param_map.count(param->name))
        {
            haserror = true;
            err.error(node.loc,"error,redeclare function parameter");
        }
        param_map[param->name] = param;
        if(param->param_type == Type::VOID)
        {
            haserror = true;
            err.error(node.loc,"error,void function parameter");
        }
    }

}

void SyntaxTreeChecker::visit(FuncDef &node) 
{
    if(declare_function(&node))
        ;
    else
    {
        haserror = true;
        err.error(node.loc,node.name+" : ReDefFunc"+",err_code : "+err_code["ReDefFunc"]+" this function has been defined");
        return;
    }
    node.param_list->accept(*this);
    hasRet = false;
    cur_func = lookup_function(node.name);
    node.body->accept(*this);
    if(!hasRet&&node.ret_type!=Type::VOID&&(node.name!=std::string("main")))
    {
        //haserror = true;
        err.warn(node.loc,node.name+" : NoRet,warn_code : "+warn_code["NoRet"]+" no return in this function");
        //return;
    }
    hasRet = false;
    cur_func = nullptr;
}
void SyntaxTreeChecker::visit(BinaryExpr &node)
{
    Type ltype;
    Type rtype;
    bool lconst;
    bool rconst;
    bool lcond;
    bool rcond;
    node.lhs->accept(*this);
    ltype = ExprType;
    lconst = isConst;
    lcond = isCond;
    if(ltype==Type::VOID)
    {
        haserror = true;
        err.error(node.loc,"type void");
        isConst = false;
        exit(-1);
    }
    if(lcond)
    {
        haserror = true;
        err.error(node.loc,"cannot use cond expr with add expr");
        isConst = false;
        exit(-1);
    }
    node.rhs->accept(*this);
    rtype = ExprType;
    rconst = isConst;
    rcond = isCond;
    if(rtype==Type::VOID)
    {
        haserror = true;
        err.error(node.loc,"type void");
        isConst = false;
        exit(-1);
    }
    if(rcond)
    {
        haserror = true;
        err.error(node.loc,"cannot use cond expr as add expr");
        isConst = false;
        exit(-1);
    }
    bool BinExprConst = lconst&&rconst;
    isConst = BinExprConst;
    ExprType = Type::INT;
    isCond = false;
}
void SyntaxTreeChecker::visit(UnaryExpr &node)
{
    Type rtype;
    bool rconst;
    bool rcond;
    node.rhs->accept(*this);
    rtype = ExprType;
    rconst = isConst;
    rcond = isCond;
    if(rtype==Type::VOID)
    {
        haserror = true;
        err.error(node.loc,"type void");
        isConst = false;
        exit(-1);
    }
    if(rcond)
    {
        haserror = true;
        err.error(node.loc,"cannot use cond expr with add expr");
        isConst = false;
        exit(-1);
    }
    isCond = false;
}
void SyntaxTreeChecker::visit(LVal &node)
{
    auto Valptr = lookup_variable(node.name);
    Type LvalType;
    bool LvalConst;
    if(Valptr==nullptr)
    {
        ExprType = Type::VOID;
        isConst = false;
        haserror = true;
        err.error(node.loc,"VarNotDefined,err_code : "+err_code["VarNotDefined"]+" variable not found");
        exit(-1);
    }
    else
    {
        LvalConst = Valptr->var_def->is_constant;
        LvalType = Valptr->var_def->btype;
        Valptr->isused = true;
        if(Valptr->var_def->array_length.size()!=node.array_index.size())
        {
            ExprType = LvalType;
            isConst = false;
            haserror = true;
            isCond = false;
            err.error(node.loc,"ArrayIndexErr,err_code : "+err_code["ArrayIndexErr"]+" imcompatible index");
            exit(-1);
        }
        for(auto exp:node.array_index)
        {
            exp->accept(*this);
            if(isCond)
            {
                err.error(node.loc,"cannot use cond expr as add expr");
                exit(-1);
            }
            if(ExprType==Type::VOID)
            {
                err.error(node.loc,"cannot use cond expr as add expr");
                exit(-1);
            }
            LvalConst = LvalConst & isConst;
        }
    }
    ExprType = LvalType;
    isConst = LvalConst;
    isCond = false;
}
void SyntaxTreeChecker::visit(Literal &node)
{
    isCond = false;
    isConst = true;
    ExprType = Type::INT;
}
void SyntaxTreeChecker::visit(ReturnStmt &node)
{
    if(cur_func==nullptr)
    {
        haserror = true;
        err.error(node.loc,"RetNotInFunc");
        exit(-1);
    }
    Type rettype;
    hasRet = true;
    if(node.ret==nullptr)
    {
        rettype = Type::VOID;
    }
    else
    {
        node.ret->accept(*this);
        rettype = ExprType;
    }
    if(rettype==Type::VOID)
    {
        if(cur_func->func_def->ret_type==Type::INT)
        {
            haserror = true;
            err.error(node.loc,"ERetType,err_code : "+warn_code["ERetType"]+" no return value in function");
            return;
        }
    }
    else if(rettype==Type::INT)
    {
        if(cur_func->func_def->ret_type==Type::VOID)
        {
            haserror = true;
            err.error(node.loc,"ERetType,err_code : "+warn_code["ERetType"]+" cannot return value in function typed void");
            return;
        }
    }
}
void SyntaxTreeChecker::visit(VarDef &node)//TODO:array support
{
    return;
}

void SyntaxTreeChecker::visit(AssignStmt &node)
{
    node.target->accept(*this);
    auto lhs = lookup_variable(node.target->name);
    if(lhs!=nullptr)
    {
        if(lhs->var_def->is_constant)
        {
            haserror = true;
            err.error(node.loc,"AssignErr,err_code : "+err_code["AssignErr"]+" read only");
            exit(-1);
        }
        else
        {
            auto ltype = lhs->var_def->btype;
            node.value->accept(*this);   
            auto rtype = ExprType;
            if(rtype==Type::VOID||ltype==Type::VOID)
            {
                haserror = true;
                err.error(node.loc,"AssignErr,err_code : "+err_code["AssignErr"]);
                exit(-1);
            }
        }
    }
}
void SyntaxTreeChecker::visit(FuncCallStmt &node) 
{
    auto FuncPtr = lookup_function(node.name);
    if(FuncPtr==nullptr)
    {
        haserror = true;
        ExprType = Type::INT;
        err.error(node.loc,"FuncNotDefined,err_code : "+err_code["FuncNotDefined"]);
        exit(-1);
    }
    if(FuncPtr->func_def->param_list->params.size()!=node.params.size())
    {
        //TODO:ERROR?OR NOT ERROR?
        err.error(node.loc,"params not match");
    }else
    {
        Type Rtype;
        auto Fparam = FuncPtr->func_def->param_list->params.begin();
        auto Rparam = node.params.begin();
        for(;Rparam!=node.params.end()&&Fparam!=FuncPtr->func_def->param_list->params.end();Fparam++,Rparam++)
        {
            Rparam->get()->accept(*this);
            Rtype = ExprType;
            if(Fparam->get()->param_type!=Rtype)
            {
                err.error(node.loc,"incompatible type");
                exit(-1);
            }
        }
    }
    isConst = false;
    isCond = false;
    ExprType = FuncPtr->func_def->ret_type;
}
void SyntaxTreeChecker::visit(BlockStmt &node)
{
    enter_scope();
    for(auto exp:node.body)
    {
        exp->accept(*this);
    }
    exit_scope(); 
}
void SyntaxTreeChecker::visit(EmptyStmt &node) 
{
    return;
}
void SyntaxTreeChecker::visit(ExprStmt &node)
{
    node.exp->accept(*this);
}

void SyntaxTreeChecker::visit(IfStmt &node)
{
    //FIXME:FINISH THIS;
    node.cond_exp->accept(*this);
    enter_scope();
    node.if_statement->accept(*this);
    exit_scope();
    if (node.else_statement != nullptr) {
        enter_scope();
        node.else_statement->accept(*this);
        exit_scope();
    }
}

void SyntaxTreeChecker::visit(WhileStmt &node)
{
    //FIXME:FINISH THIS;
    node.cond_exp->accept(*this);
    enter_scope();
    node.statement->accept(*this);
    StmtStack.back() = node.statement;
    exit_scope();
}

void SyntaxTreeChecker::visit(BreakStmt &node)
{
    //FIXME:FINISH THIS;
    if (StmtStack.back() == nullptr) {
        haserror = true;
        err.error(node.loc,"UnmatchedBreak,err_code : "+err_code["UnmatchedBreak"]);
    }
}

void SyntaxTreeChecker::visit(ContinueStmt &node)
{
    //FIXME:FINISH THIS;
    if (StmtStack.back() == nullptr) {
        haserror = true;
        err.error(node.loc,"UnmatchedContinue,err_code : "+err_code["UnmatchedContinue"]);
    }
}

void SyntaxTreeChecker::visit(BinaryCondExpr &node)
{
    node.lhs->accept(*this);
    Type ltype = ExprType;
    bool lcond = isCond;
    node.rhs->accept(*this);
    Type rtype = ExprType;
    bool rcond = isCond;
    if(node.op==BinaryCondOp::LOR||node.op==BinaryCondOp::LAND)
    {
        if(ltype==Type::VOID||rtype==Type::VOID)
        {
            err.error(node.loc,"type void");
            exit(-1);
        }
    }else{
        if(lcond||rcond)
        {
            err.error(node.loc,"cannot use CondExp in EqExp");
            exit(-1);
        }
        if(ltype==Type::VOID||rtype==Type::VOID)
        {
            err.error(node.loc,"type void");
            exit(-1);
        }
    }
    isCond = true;
    isConst = false;
}

void SyntaxTreeChecker::visit(UnaryCondExpr &node)
{
    node.rhs->accept(*this);
    Type rtype = ExprType;
    bool rcond = isCond;
    if(!rcond)
    {
        err.error(node.loc,"NOT can only be used in CondExp");
        exit(-1);
    }
    if(rtype==Type::VOID)
    {
        err.error(node.loc,"type void");
        exit(-1);
    }
    isCond = true;
    isConst = false;
}

void SyntaxTreeChecker::visit(InitVal &node)
{
    return;
    //TODO:FINISH THIS;
}