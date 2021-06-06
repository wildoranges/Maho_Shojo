#ifndef _MHSJ_SYNTAX_TREE_CHECKER_H_
#define _MHSJ_SYNTAX_TREE_CHECKER_H_

#include "SyntaxTree.h"
#include "ErrorReporter.h"
#include <cassert>
#include <vector>

using SyntaxTree::Ptr;
using SyntaxTree::PtrList;
using SyntaxTree::BlockStmt;
using SyntaxTree::Expr;
using SyntaxTree::FuncFParamList;
using SyntaxTree::VarDef;
using SyntaxTree::FuncDef;

class SyntaxTreeChecker : public SyntaxTree::Visitor
{
public:
    bool is_err()
    {
        return haserror;
    }
    SyntaxTreeChecker(ErrorReporter &e) :err(e) {
        err_code["ReDefVal"] = std::string("00");
        err_code["FuncDefInFunc"] = std::string("01");
        err_code["ReDefFunc"] = std::string("02");
        err_code["RetNotInFunc"] = std::string("03");
        err_code["ERetType"] = std::string("04");
        err_code["NoRet"]  = std::string("05");
        err_code["DivZero"] = std::string("06");
        err_code["InValidOperands"] = std::string("07");
        err_code["ImproperArrayDef"] = std::string("08");
        err_code["IncompatibleVarInit"] = std::string("09");
        err_code["IncompatibleArrayInit"] = std::string("10");
        err_code["VarNotDefined"] = std::string("11");
        err_code["ArrayOutOfRange"] = std::string("12");
        err_code["ArrayIndexErr"] = std::string("13");
        err_code["AssignErr"] = std::string("14");
        err_code["FuncNotDefined"] = std::string("15");
        err_code["ValDefinedVoid"] = std::string("16");
        err_code["TypeVoid"] = std::string("17");
        err_code["UnmatchedBreak"] = std::string("18");
        err_code["UnmatchedContinue"] = std::string("19");

        warn_code["WMainFunc"] = std::string("49");
        warn_code["WRetType"] = std::string("50");
        warn_code["IncompatibleArrayInit"] = std::string("51");
        warn_code["IncompatibleVarInit"] = std::string("52");
        warn_code["UnusedVar"] = std::string("53");
        warn_code["NoMainFunc"] = std::string("54");
        warn_code["ERetType"] = std::string("55");
        warn_code["TypeConvert"] = std::string("56");
        warn_code["NoRet"] = std::string("57");
    }
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
    virtual void visit(SyntaxTree::IfStmt &node) override;
    virtual void visit(SyntaxTree::WhileStmt &node) override;
    virtual void visit(SyntaxTree::BreakStmt &node) override;
    virtual void visit(SyntaxTree::ContinueStmt &node) override;
    virtual void visit(SyntaxTree::BinaryCondExpr &node)override;
    virtual void visit(SyntaxTree::UnaryCondExpr &node)override;
    virtual void visit(SyntaxTree::InitVal &node) override;
private:
    std::vector<Ptr<SyntaxTree::Stmt>> StmtStack;//used for WhileStmt
    bool haserror = false;
    using Type = SyntaxTree::Type;
    bool hasRet = false;
    bool isConst = false;
    bool isCond = false;
    Type ExprType = SyntaxTree::Type::VOID;
    class Variable
    {
    public:
        bool isused = false;
        Ptr<VarDef> var_def;

        Variable() = default;
        Variable(VarDef* def)
        {
            var_def = Ptr<VarDef>(def);
        }
    };

    class Function
    {
    public:
        bool isused = false;
        Ptr<FuncDef> func_def;

        Function() = default;
        Function(FuncDef* def)
        {
            func_def = Ptr<FuncDef>(def);
        }
    };

    using PtrFunction = std::shared_ptr<Function>;

    using PtrVariable = std::shared_ptr<Variable>;

    PtrFunction cur_func = nullptr;
    
    void enter_scope() 
    { 
        variables.emplace_front(); 
        StmtStack.push_back(nullptr);
    }

    void exit_scope() 
    { 
        for(auto var:variables.front())
        {
            if(var.second->isused == false)
            {
                std::cout << " warning: variable '"<<var.first<<"' not used,warn_code : "<<warn_code["UnusedVar"]<<std::endl;
            }
        }
        variables.pop_front(); 
        StmtStack.pop_back();
    }

    PtrVariable lookup_variable(std::string& name)
    {
        for (auto m : variables)
            if (m.count(name))
                return m[name];
        return nullptr;
    }

    PtrFunction lookup_function(std::string& name)
    {
        if(functions.count(name))
        {
            return functions[name];
        }
        return nullptr;
    }

    bool declare_variable(VarDef* var_def)
    {
        assert(!variables.empty());
        if (variables.front().count(var_def->name))
            return false;
        variables.front()[var_def->name] = PtrVariable(new Variable(var_def));
        return true;
    }

    bool declare_function(FuncDef* func_def)
    {
        if(functions.count(func_def->name))
            return false;
        functions[func_def->name] = PtrFunction(new Function(func_def));
        return true;
    }
    std::deque<std::unordered_map<std::string, PtrVariable>> variables;

    std::unordered_map<std::string, PtrFunction> functions;
    
    std::unordered_map<std::string, std::string> err_code;

    std::unordered_map<std::string, std::string> warn_code;

    ErrorReporter &err;
};

#endif  // _MHSJ_SYNTAX_TREE_CHECKER_H_
