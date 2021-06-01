#ifndef _C1_SYNTAX_TREE_CHECKER_H_
#define _C1_SYNTAX_TREE_CHECKER_H_

#include "SyntaxTree.h"
#include "ErrorReporter.h"
#include <cassert>

using SyntaxTree::Ptr;
using SyntaxTree::PtrList;
using SyntaxTree::BlockStmt;
using SyntaxTree::Expr;

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
        warn_code["WMainFunc"] = std::string("18");

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
private:
    bool haserror = false;
    using Type = SyntaxTree::Type;
    bool inFunc = false;//useless,ignore this
    bool hasRet = false;
    bool isConst = false;
    int INTConstVal = 0;
    double FLTConstVal = 0.0;
    Type FuncType = SyntaxTree::Type::VOID;
    Type ExprType = SyntaxTree::Type::VOID;
    class Variable
    {
    public:
        const bool is_const = false;
        bool isused = false;
        Type btype;
        std::vector<int> array_length;
        PtrList<Expr> initializers;

        Variable() = default;
        Variable(Type type, std::vector<int>& array,PtrList<Expr>& init)
        {
            btype = type;
            array_length = array;
            initializers = init;
        }
        virtual bool isconst()
        {
            return false;
        }
    };

    class ConstVar: public Variable
    {
    public:
        const bool is_const = true;
        ConstVar(Type type, std::vector<int>& array,PtrList<Expr>& init)
        {
            btype = type;
            array_length = array;
            initializers = init;
        }
        bool isconst() override
        {
            return true;
        }
    };

    struct Function
    {
        Type ret_type;
        Ptr<BlockStmt> body;
        Function(Type t,Ptr<BlockStmt>& b):ret_type(t),body(b) {}
    };

    using PtrFunction = std::shared_ptr<Function>;

    using PtrVariable = std::shared_ptr<Variable>;
    
    void enter_scope() 
    { 
        variables.emplace_front(); 
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

    bool declare_variable(std::string& name, bool is_const, Type type, std::vector<int>& array,PtrList<Expr>& init)
    {
        assert(!variables.empty());
        if (variables.front().count(name))
            return false;
        //variables.front()[name] = PtrVariable(new Variable(is_const, btype, array_length));
        if(is_const)
        {
            variables.front()[name] = PtrVariable(new ConstVar(type,array,init));
        }
        else
        {
            variables.front()[name] = PtrVariable(new Variable(type,array,init));
        }
        return true;
    }

    bool declare_function(std::string& name,SyntaxTree::Type t ,Ptr<BlockStmt> b)
    {
        if(functions.count(name))
            return false;
        functions[name] = PtrFunction(new Function(t,b));
        return true;
    }
    std::deque<std::unordered_map<std::string, PtrVariable>> variables;

    std::unordered_map<std::string, PtrFunction> functions;
    
    std::unordered_map<std::string, std::string> err_code;

    std::unordered_map<std::string, std::string> warn_code;

    ErrorReporter &err;
};

#endif  // _C1_SYNTAX_TREE_CHECKER_H_
