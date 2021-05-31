#include "SyntaxTreeChecker.h"
#include <system_error>

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
        //err.warn(node.loc,"NoMainFunc,warn_code : "+warn_code["NoMainFunc"]);
    }
    else
    {
        auto Funcname = std::string("main");
        auto FuncP = lookup_function(Funcname);
        if(FuncP->ret_type!=Type::INT)
        {
            haserror = true;
            std::cout<<"error,WMainFunc,err_code: "<<warn_code["WMainFunc"]<<" expect return int\n";
        }
    }
    exit_scope();
}


void SyntaxTreeChecker::visit(FuncDef &node) 
{
    if(inFunc)
    {
        /*haserror = true;
        err.error(node.loc,node.name+" : FuncDefInFunc"+",err_code : "+err_code["FuncDefInFunc"]+" cannot define function in function");
        return;*/
    }
    if(declare_function(node.name,node.ret_type,node.body))
        ;
    else
    {
        haserror = true;
        err.error(node.loc,node.name+" : ReDefFunc"+",err_code : "+err_code["ReDefFunc"]+" this function has been defined");
        return;
    }
    inFunc = true;
    FuncType = node.ret_type;
    hasRet = false;
    node.body->accept(*this);
    if(!hasRet&&node.ret_type!=Type::VOID&&(node.name!=std::string("main")))
    {
        //haserror = true;
        err.warn(node.loc,node.name+" : NoRet,warn_code : "+warn_code["NoRet"]+" no return in this function");
        //return;
    }
    inFunc = false;
    FuncType = SyntaxTree::Type::VOID;
    hasRet = false;
}
void SyntaxTreeChecker::visit(BinaryExpr &node)
{
    Type ltype;
    Type rtype;
    bool lconst;
    bool rconst;
    int int_l_val,int_r_val;
    double flt_l_val,flt_r_val;
    node.lhs->accept(*this);
    ltype = ExprType;
    lconst = isConst;
    if(ltype==Type::VOID)
    {
        haserror = true;
        err.error(node.loc,"type void");
        isConst = false;
        return;
    }
    if(ltype==Type::INT&&lconst)
    {
        int_l_val = INTConstVal;
    }
    else if(ltype==Type::FLOAT&&lconst)
    {
        flt_l_val = FLTConstVal;
    }
    node.rhs->accept(*this);
    rtype = ExprType;
    rconst = isConst;
    if(rtype==Type::VOID)
    {
        haserror = true;
        err.error(node.loc,"type void");
        isConst = false;
        return;
    }
    if(rtype==Type::INT&&rconst)
    {
        int_r_val = INTConstVal;
    }
    else if(rtype==Type::FLOAT&&rconst)
    {
        flt_r_val = FLTConstVal;
    }
    bool BinExprConst = lconst&&rconst;
    isConst = BinExprConst;
    int INTBinExprVal;
    double FLTBinExprval;
    Type BinExprType;
    if((ltype==Type::FLOAT||rtype==Type::FLOAT)&&BinExprConst)
    {
        if(ltype==Type::INT)
        {
            flt_l_val = (double)int_l_val;
        }
        if(rtype==Type::INT)
        {
            flt_r_val = (double)int_r_val;
        }
    }
    if(ltype==Type::INT&&rtype==Type::INT)
    {
        BinExprType = Type::INT;
        ExprType = BinExprType;
        if(BinExprConst)
        {
            switch (node.op)
            {
                case (BinOp::PLUS):
                {
                    INTBinExprVal = int_l_val + int_r_val;
                }break;
                case (BinOp::DIVIDE):
                {
                    if(int_r_val==0)
                    {
                        isConst = false;
                        haserror = true;
                        err.error(node.loc,"DivZero,err_code : "+err_code["DivZero"]+" cannot divide 0");
                        return;
                    }
                    else
                    {
                        INTBinExprVal = int_l_val / int_r_val;
                    }
                }break;
                case (BinOp::MINUS):
                {
                    INTBinExprVal = int_l_val - int_r_val;
                }break;
                case(BinOp::MODULO):
                {
                    if(int_r_val==0)
                    {
                        isConst = false;
                        haserror = true;
                        err.error(node.loc,"DivZero,err_code : "+err_code["DivZero"]+" cannot divide 0");
                        return;
                    }
                    else
                    {
                        INTBinExprVal = int_l_val % int_r_val;
                    }
                }break;
                case(BinOp::MULTIPLY):
                {
                    INTBinExprVal = int_l_val * int_r_val;
                }break;
            }
            INTConstVal = INTBinExprVal;
        }
    }
    else
    {
        BinExprType = Type::FLOAT;
        ExprType = BinExprType;
        if(BinExprConst)
        {
            switch (node.op)
            {
                case(BinOp::PLUS):
                {
                    FLTBinExprval = flt_l_val + flt_r_val;
                }break;
                case(BinOp::MINUS):
                {
                    FLTBinExprval = flt_l_val - flt_r_val;
                }break;
                case(BinOp::MULTIPLY):
                {
                    FLTBinExprval = flt_l_val * flt_r_val;
                }break;
                case(BinOp::MODULO):
                {
                    /*if(flt_r_val==0.0)
                    {
                        haserror = true;
                        err.error(node.loc,"DivZero,err_code : "+err_code["DivZero"]+" cannot divide 0");
                        exit(1);
                    }
                    */
                    isConst = false;
                    haserror = true;
                    err.error(node.loc,"InValidOperands,err_code : "+err_code["InValidOperands"]+" invalid operands between '%' ");
                    return;
                }break;
                case(BinOp::DIVIDE):
                {
                    if(flt_r_val==0.0)
                    {
                        isConst = false;
                        haserror = true;
                        err.error(node.loc,"DivZero,err_code : "+err_code["DivZero"]+" cannot divide 0");
                        return;
                    }
                    else
                    {
                        FLTBinExprval = flt_l_val / flt_r_val;
                    }
                }break;
            }
            FLTConstVal = FLTBinExprval;
        }
    }
    ExprType = BinExprType;
    isConst = BinExprConst;
}
void SyntaxTreeChecker::visit(UnaryExpr &node)
{
    Type rtype;
    bool rconst;
    //int int_r_val;
    //double flt_r_val;
    node.rhs->accept(*this);
    rtype = ExprType;
    rconst = isConst;
    int INTUnExprVal;
    double FLTUnExprVal;
    bool UnExprConst = rconst;
    isConst = UnExprConst;
    Type UnExprType;
    if(rtype==Type::INT)
    {
        UnExprType = Type::INT;
        ExprType = UnExprType;
        if(UnExprConst)
        {
            switch (node.op)
            {
                case(UnaryOp::PLUS):
                {
                    INTUnExprVal = INTConstVal;
                }break;
                case(UnaryOp::MINUS):
                {
                    INTUnExprVal = - INTConstVal;
                }break;
            }
            INTConstVal = INTUnExprVal;
        }
    }
    else if(rtype==Type::FLOAT)
    {
        UnExprType = Type::FLOAT;
        ExprType = UnExprType;
        if(UnExprConst)
        {
            switch (node.op)
            {
                case(UnaryOp::PLUS):
                {
                    FLTUnExprVal = FLTConstVal;
                }break;
                case(UnaryOp::MINUS):
                {
                    FLTUnExprVal = - FLTConstVal;
                }break;
            }
            FLTConstVal = FLTUnExprVal;
        }
    }
    else
    {
        ExprType = Type::VOID;
        isConst = false;
        haserror = true;
        err.error(node.loc,"InValidOperands,err_code : "+err_code["InValidOperands"]+" not expected type void");
        return;
    }
    isConst = UnExprConst;
    ExprType = UnExprType;
}
void SyntaxTreeChecker::visit(LVal &node)
{
    auto Valptr = lookup_variable(node.name);
    bool LvalConst;
    Type LvalType;
    //int int_Lval;
    //double flt_Lval;
    if(Valptr==nullptr)
    {
        ExprType = Type::VOID;
        isConst = false;
        haserror = true;
        err.error(node.loc,"VarNotDefined,err_code : "+err_code["VarNotDefined"]+" variable not found");
        return;
    }
    else if(!Valptr->isconst())
    {
        LvalType = Valptr->btype;
        LvalConst = false;
        Valptr->isused = true;
        if(!node.array_index.empty())
        {
            if(Valptr->array_length.size()!=node.array_index.size())
            {
                ExprType = LvalType;
                isConst = false;
                haserror = true;
                err.error(node.loc,"ArrayIndexErr,err_code : "+err_code["ArrayIndexErr"]+" imcompatible index");   
                return;
            }
            std::vector<int>::iterator lenbeg = Valptr->array_length.begin();
            std::vector<Ptr<Expr>>::iterator expbeg = node.array_index.begin();
            for(;lenbeg!=Valptr->array_length.end();lenbeg++)
            {
                if(expbeg==node.array_index.end())
                {
                    break;
                }
                int maxlen = *lenbeg;
                auto expr = *expbeg;
                expr->accept(*this);
                if(isConst)
                {
                    if(ExprType==Type::INT)
                    {
                        if(INTConstVal>=maxlen||INTConstVal<0)
                        {
                            ExprType = LvalType;
                            isConst = false;
                            haserror = true;
                            err.error(node.loc,"ArrayOutOfRange,err_code : "+err_code["ArrayOutOfRange"]);
                            return;
                        }
                    }
                    else
                    {
                        ExprType = LvalType;
                        isConst = false;
                        haserror = true;
                        err.error(node.loc,"ArrayIndexErr,err_code : "+err_code["ArrayIndexErr"]+" expect int");   
                        return;
                    }
                }
                else
                {
                    if(ExprType!=Type::INT)
                    {
                        ExprType = LvalType;
                        isConst = false;
                        haserror = true;
                        err.error(node.loc,"ArrayIndexErr,err_code : "+err_code["ArrayIndexErr"]+" expect int");   
                        return;
                    }
                }
                expbeg++;
            }
            /*if(expbeg!=node.array_index.end())
            {
                ExprType = LvalType;
                isConst = false;
                err.error(node.loc,"ArrayIndexErr,err_code : "+err_code["ArrayIndexErr"]+" out of range");
                return;
            }*/
        }
        else
        {
            if(Valptr->array_length.empty())
            {
                LvalType = Valptr->btype;
            }
            else
            {
                ExprType = LvalType;
                isConst = false;
                haserror = true;
                err.error(node.loc,"ArrayIndexErr,err_code : "+err_code["ArrayIndexErr"]+" incompatible index");
                return;
            }
        }
    }
    else
    {
        LvalType = Valptr->btype;
        Valptr->isused = true;
        LvalConst = true;
        for(auto expr:Valptr->initializers)
        {
            expr->accept(*this);
            Type inittype = ExprType;
            //ExprType = Valptr->btype;
            if(inittype==Type::INT)
            {
                if(Valptr->btype==Type::INT)
                {

                }
                else
                {
                    FLTConstVal = (double)INTConstVal;
                }
            }
            else if(inittype==Type::FLOAT)
            {
                if(Valptr->btype==Type::FLOAT)
                {

                }
                else
                {
                    INTConstVal = (int)FLTConstVal;
                }
            }
            else
            {
                haserror = true;
                exit(1);
            }
        }
    }
    ExprType = LvalType;
    isConst = LvalConst;
}
void SyntaxTreeChecker::visit(Literal &node)
{
    isConst = true;
    if(node.is_int)
    {
        ExprType = Type::INT;
        INTConstVal = node.int_const;
    }
    else
    {
        ExprType = Type::FLOAT;
        FLTConstVal = node.float_const;
    }
}
void SyntaxTreeChecker::visit(ReturnStmt &node)
{
    /*if(!inFunc)
    {
        haserror = true;
        err.error(node.loc,"RetNotInFunc,err_code : "+err_code["RetNotInFunc"]+" keyword 'return' must be used in a function");
        return;
    }*/
    Type rettype;
    if(node.ret==nullptr)
    {
        rettype = Type::VOID;
    }
    else
    {
        node.ret->accept(*this);
        rettype = ExprType;
    }
    hasRet = true;
    if(rettype==Type::VOID)
    {
        if(FuncType==Type::FLOAT||FuncType==Type::INT)
        {
            haserror = true;
            err.error(node.loc,"ERetType,err_code : "+warn_code["ERetType"]+" no return value in function");
            return;
        }
    }
    else if(rettype==Type::INT)
    {
        if(FuncType==Type::VOID)
        {
            haserror = true;
            err.error(node.loc,"ERetType,err_code : "+warn_code["ERetType"]+" cannot return value in function typed void");
            return;
        }
        else if(FuncType==Type::FLOAT)
        {
            err.warn(node.loc,"WRetType,warn_code : "+warn_code["WRetType"]+" convert int to float");
        }
    }
    else
    {
        if(FuncType==Type::VOID)
        {
            haserror = true;
            err.error(node.loc,"ERetType,err_code : "+warn_code["ERetType"]+" cannot return value in function typed void");
            return;
        }
        else if(FuncType==Type::INT)
        {
            err.warn(node.loc,"WRetType,warn_code : "+warn_code["WRetType"]+" convert float to int");
        }
    }
}
void SyntaxTreeChecker::visit(VarDef &node)
{
    if (variables.front().count(node.name))
    {
        haserror = true;
        err.error(node.loc,node.name+" : ReDefVal"+",err_code : "+err_code["ReDefVal"]+" this variable has been defined");
        //exit(1);
        return;
    }
    if(node.btype == Type::VOID)
    {
        haserror = true;
        err.error(node.loc,node.name+" : ValDefinedVoid,err_code : "+err_code["ValDefinedVoid"]);
        return;
    }
    else {
        //isConst = node.is_constant;
        auto parray = new std::vector<int>;
        parray->clear();
        if(!node.array_length.empty())
        {
            int arrlen = 1;
            for (auto exp : node.array_length) {
                exp->accept(*this);
                if(!isConst)
                {
                    haserror = true;
                    err.error(node.loc," : ImproperArrayDef,err_code : "+err_code["ImproperArrayDef"]+" Expr must be const");
                    return;
                }
                else
                {
                    if(ExprType!=Type::INT||INTConstVal<0)
                    {
                        haserror = true;
                        err.error(node.loc," : ImproperArrayDef,err_code : "+err_code["ImproperArrayDef"]+" Expr must be int(>=0)");
                        return;
                    }
                    else
                    {
                        parray->push_back(INTConstVal);
                        arrlen *= INTConstVal;
                    }
                }
            }
            int initlen = node.initializers.size();
            if(initlen>0&&initlen>arrlen)
            {
                haserror = true;
                err.error(node.loc," : IncompatibleArrayInit,err_code : "+err_code["IncompatibleArrayInit"]+" init length dismatch array length");
                return;
            }
            for (auto exp : node.initializers) {
                exp->accept(*this);
                /*if(!isConst&&node.is_constant)
                {
                    
                    haserror = true;
                    err.error(node.loc," : ImproperArrayDef,err_code : "+err_code["ImproperArrayDef"]+" Expr must be const");
                    return;
                }
                else*/
                {
                    if(ExprType!=node.btype&&ExprType!=Type::VOID)
                    {
                        //haserror = true;
                        err.warn(node.loc," : TypeConvert,warn_code : "+warn_code["TypeConvert"]+" init type dismatch the array type");
                        //return;
                    }
                    else if(ExprType==Type::VOID)
                    {
                        haserror = true;
                        err.error(node.loc," : AssignErr,err_code : "+err_code["AssignErr"]);
                        return;
                    }   
                }
            }
            declare_variable(node.name,node.is_constant,node.btype,*parray,node.initializers);
        }
        else 
        {
            int initlen = node.initializers.size();
            if(initlen>1)
            {
                haserror = true;
                err.error(node.loc," : IncompatibleVarInit,err_code : "+err_code["IncompatibleVarInit"]+" at most 1 expr");
                return;
            }
            else
            {
                //int constint;
                //double constflt;
                for(auto exp:node.initializers)
                {
                    exp->accept(*this);
                    if(!isConst&&node.is_constant)
                    {
                        haserror = true;
                        err.error(node.loc," : IncompatibleVarInit,err_code : "+err_code["IncompatibleVarInit"]+" not a const");
                        return;
                    }
                    else
                    {
                        if(ExprType!=node.btype&&ExprType!=Type::VOID)
                        {
                            //haserror = true;
                            err.warn(node.loc," : TypeConvert,warn_code : "+warn_code["TypeConvert"]+" init type dismatch the array type");
                            //return;
                        }
                        if(ExprType==Type::VOID)
                        {
                            haserror = true;
                            err.error(node.loc," : AssignErr,err_code : "+err_code["AssignErr"]);
                            return;
                        }
                    }
                    /*else
                    {
                        if(ExprType!=node.btype&&ExprType!=Type::VOID)
                        {
                        //haserror = true;
                            err.warn(node.loc," : TypeConvert,warn_code : "+warn_code["TypeConvert"]+" init type dismatch the array type");
                        //return;
                        }
                        else if(ExprType==Type::VOID)
                        {
                            haserror = true;
                            err.error(node.loc," : AssignErr,err_code : "+err_code["AssignErr"]);
                            return;
                        }
                    }*/
                }
                declare_variable(node.name,node.is_constant,node.btype,*parray,node.initializers);
            }
            /*else
            {
                for(auto exp:node.initializers)
                {
                    exp->accept(*this);
                    if(ExprType!=node.btype)
                    {
                        haserror = true;
                        err.error(node.loc," : IncompatibleVarInit,err_code : "+err_code["IncompatibleVarInit"]+" init type dismatch the var type");
                        return;
                    }
                }
                declare_variable(node.name,node.is_constant,node.btype,*parray,node.initializers);
            }*/
        }
    }
}
void SyntaxTreeChecker::visit(AssignStmt &node)
{
    node.target->accept(*this);
    auto lhs = lookup_variable(node.target->name);
    if(lhs!=nullptr)
    {
        if(lhs->isconst())
        {
            haserror = true;
            err.error(node.loc,"AssignErr,err_code : "+err_code["AssignErr"]+" read only");
            return;
        }
        else
        {
            auto ltype = lhs->btype;
            node.value->accept(*this);   
            auto rtype = ExprType;
            if(ltype!=rtype&&rtype!=Type::VOID)
            {
                //haserror = true;
                err.warn(node.loc,"TypeConvert,warn_code : "+warn_code["TypeConvert"]+" type between '=' not match");
                //return;
            }
            else if(rtype==Type::VOID)
            {
                haserror = true;
                err.error(node.loc,"AssignErr,err_code : "+err_code["AssignErr"]);
                return;
            }
        }
    }
}
void SyntaxTreeChecker::visit(FuncCallStmt &node) 
{
    auto FuncPtr = lookup_function(node.name);
    isConst = false;
    if(FuncPtr==nullptr)
    {
        haserror = true;
        ExprType = Type::INT;
        err.error(node.loc,"FuncNotDefined,err_code : "+err_code["FuncNotDefined"]);
    }
    else
    {
        ExprType = FuncPtr->ret_type;   
    }
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

void SyntaxTreeChecker::visit(FuncParam &node) 
{
    return;//TODO:FINISH THIS;
}

void SyntaxTreeChecker::visit(FuncFParamList &node)
{
    return;//TODO:FINISH THIS;
}
