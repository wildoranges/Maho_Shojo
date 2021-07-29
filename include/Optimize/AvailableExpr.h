//
// Created by cjb on 7/28/21.
//

#ifndef MHSJ_AVALIABLEEXPR_H
#define MHSJ_AVALIABLEEXPR_H

#include "Pass.h"
#include <map>
#include <set>
#include "Instruction.h"

struct cmp_expr{
    bool operator()(Instruction* a,Instruction* b)const{
        auto a_oprs = a->get_operands();
        auto b_oprs = b->get_operands();
        auto a_size = a_oprs.size();
        auto b_size = b_oprs.size();
        if(a_size < b_size){
            return true;
        }
        if(a_size > b_size){
            return false;
        }
        auto a_iter = a_oprs.begin();
        auto b_iter = b_oprs.begin();
        while (a_iter!=a_oprs.end()){
            auto a_operand = *a_iter;
            auto b_operand = *b_iter;//TODO:OPR ORDER?
            if(a_operand!=b_operand){
                auto const_a_opr = dynamic_cast<ConstantInt*>(a_operand);
                auto const_b_opr = dynamic_cast<ConstantInt*>(b_operand);
                if(const_a_opr&&const_b_opr){
                    if(const_a_opr->get_value()!=const_b_opr->get_value()){
                        return (a_operand->get_name() < b_operand->get_name());
                    }
                }else{
                    return (a_operand->get_name() < b_operand->get_name());
                }
            }
            a_iter++;
            b_iter++;
        }
        if(a->get_instr_op_name()!=b->get_instr_op_name()){
            return (a->get_instr_op_name() < b->get_instr_op_name());
        }
        return false;
    }
};

class AvailableExpr: public Pass{
public:
    explicit AvailableExpr(Module* module):Pass(module){}
    void execute() override;
    void compute_local_gen(Function* f);
    //void compute_local_kill(Function* f);
    void compute_global_in_out(Function* f);
    void compute_global_common_expr(Function* f);
    void initial_map(Function* f);
    static bool is_valid_expr(Instruction* inst);
//    static void remove_relevant_instr(Value* val,std::set<Instruction*,cmp_expr>& bb_set);
//    static void insert_relevant_instr(Value* val,std::set<Instruction*,cmp_expr>& bb_set);
    const std::string name = "AvailableExpr";
private:
    std::set<Instruction*,cmp_expr> U;
    std::map<BasicBlock*,std::set<Instruction*,cmp_expr>> bb_in;
    std::map<BasicBlock*,std::set<Instruction*,cmp_expr>> bb_out;
    std::map<BasicBlock*,std::set<Instruction*,cmp_expr>> bb_gen;
    //std::map<BasicBlock*,std::set<Instruction*,cmp_expr>> bb_kill;
};

#endif //MHSJ_AVALIABLEEXPR_H
