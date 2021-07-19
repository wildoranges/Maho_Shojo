#ifndef _MHSJ_BUILDER_HPP_
#define _MHSJ_BUILDER_HPP_
#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"
#include <map>
#include "SyntaxTree.h"


class Scope {
public:
    // enter a new scope
    void enter() {
        inner.push_back({});
        inner_array_size.push_back({});
    }

    // exit a scope
    void exit() {
        inner.pop_back();
        inner_array_size.pop_back();
    }

    bool in_global() {
        return inner.size() == 1;
    }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(std::string name, Value *val) {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    Value* find(std::string name) {
        for (auto s = inner.rbegin(); s!= inner.rend();s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }

        return nullptr;
    }

    bool push_size(std::string name, std::vector<int> size){
        auto result = inner_array_size[inner_array_size.size() - 1].insert({name,size});
        /*for (auto size_one: size){
            inner_array_size[inner_array_size.size() - 1][name].push_back(size_one);
        }*/
        return result.second;
    }

    std::vector<int> find_size(std::string name) {
        for (auto s = inner_array_size.rbegin(); s!=inner_array_size.rend(); s++){
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }

        return {};
    }

private:
    std::vector<std::map<std::string, Value *>> inner;
    std::vector<std::map<std::string, std::vector<int>>> inner_array_size;
};

class MHSJBuilder: public SyntaxTree::Visitor
{
private:
    virtual void visit(SyntaxTree::InitVal &) override final;
    virtual void visit(SyntaxTree::Assembly &) override final;
    virtual void visit(SyntaxTree::FuncDef &) override final;
    virtual void visit(SyntaxTree::VarDef &) override final;
    virtual void visit(SyntaxTree::AssignStmt &) override final;
    virtual void visit(SyntaxTree::ReturnStmt &) override final;
    virtual void visit(SyntaxTree::BlockStmt &) override final;
    virtual void visit(SyntaxTree::EmptyStmt &) override final;
    virtual void visit(SyntaxTree::ExprStmt &) override final;
    virtual void visit(SyntaxTree::UnaryCondExpr &) override final;
    virtual void visit(SyntaxTree::BinaryCondExpr &) override final;
    virtual void visit(SyntaxTree::BinaryExpr &) override final;
    virtual void visit(SyntaxTree::UnaryExpr &) override final;
    virtual void visit(SyntaxTree::LVal &) override final;
    virtual void visit(SyntaxTree::Literal &) override final;
    virtual void visit(SyntaxTree::FuncCallStmt &) override final;
    virtual void visit(SyntaxTree::FuncParam &) override final;
    virtual void visit(SyntaxTree::FuncFParamList &) override final;
    virtual void visit(SyntaxTree::IfStmt &) override final;
    virtual void visit(SyntaxTree::WhileStmt &) override final;
    virtual void visit(SyntaxTree::BreakStmt &) override final;
    virtual void visit(SyntaxTree::ContinueStmt &) override final;

    IRBuilder *builder;
    Scope scope;
    std::unique_ptr<Module> module;
public:
    MHSJBuilder(){
        module = std::unique_ptr<Module>(new Module("SysY code"));
        builder = new IRBuilder(nullptr, module.get());
        auto TyVoid = Type::get_void_type(module.get());
        auto TyInt32 = Type::get_int32_type(module.get());

        auto input_type = FunctionType::get(TyInt32, {});
        auto input_fun =
            Function::create(
                    input_type,
                    "input",
                    module.get());

        std::vector<Type *> output_params;
        output_params.push_back(TyInt32);
        auto output_type = FunctionType::get(TyVoid, output_params);
        auto output_fun =
            Function::create(
                    output_type,
                    "output",
                    module.get());

        std::vector<Type *> output_float_params;

        auto neg_idx_except_type = FunctionType::get(TyVoid, {});
        auto neg_idx_except_fun =
            Function::create(
                    neg_idx_except_type,
                    "neg_idx_except",
                    module.get());

        scope.enter();
        scope.push("input", input_fun);
        scope.push("output", output_fun);
        scope.push("neg_idx_except", neg_idx_except_fun);
    }
    std::unique_ptr<Module> getModule() {
        return std::move(module);
    }
};


#endif