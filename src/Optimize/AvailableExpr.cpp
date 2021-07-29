//
// Created by cjb on 7/28/21.
//

#include "AvailableExpr.h"
#include <set>
#include <algorithm>

void AvailableExpr::execute() {
    for(auto func:module->get_functions()){
        if(func->get_basic_blocks().empty()){
            continue;
        }
        initial_map(func);
        compute_local_gen(func);
        //compute_local_kill(func);
        compute_global_in_out(func);
        compute_global_common_expr(func);
    }
}

bool AvailableExpr::is_valid_expr(Instruction *inst) {
    return !(inst->is_void()||inst->is_call()||inst->is_phi()||inst->is_alloca()||inst->is_load());//TODO:CHECK VALID INST
}

void AvailableExpr::compute_local_gen(Function *f) {
    auto all_bbs = f->get_basic_blocks();
    for(auto bb:all_bbs){
        auto instrs = bb->get_instructions();
        for(auto instr_iter=instrs.begin();instr_iter!=instrs.end();instr_iter++){
            auto instr = *instr_iter;
            if(is_valid_expr(instr)){
                auto res = bb_gen[bb].insert(instr);
                if(!res.second){
                    auto old_instr = bb_gen[bb].find(instr);
                    instr->replace_all_use_with(*old_instr);
                    instr_iter = instrs.erase(instr_iter);
                    instr_iter --;
                    instr->remove_use_of_ops();
                }else{
                    U.insert(instr);
                }
                //remove_relevant_instr(instr,bb_gen[bb]);
            }
        }
    }
}

//void AvailableExpr::compute_local_kill(Function *f) {
//    auto all_bbs = f->get_basic_blocks();
//    for(auto bb:all_bbs){
//        auto instrs = bb->get_instructions();
//        for(auto instr:instrs){
//            if(is_valid_expr(instr)){
//                bb_kill[bb].erase(instr);//TODO:CHECK ACC
//                insert_relevant_instr(instr,bb_kill[bb]);
//            }
//        }
//    }
//}

void AvailableExpr::compute_global_in_out(Function *f) {
    auto all_bbs = f->get_basic_blocks();
    auto entry = f->get_entry_block();
    for(auto bb:all_bbs){
        if(bb!=entry){
            bb_out[bb] = U;
        }
    }
    bb_out[entry] = std::set<Instruction*,cmp_expr>();
    bool change = true;
    while (change){
        change = false;
        for(auto bb:all_bbs){
            if(bb!=entry){
                std::set<Instruction*,cmp_expr> last_tmp;
                bool is_first = true;
                for(auto pred:bb->get_pre_basic_blocks()){
                    if(!is_first){
                        std::set<Instruction*,cmp_expr> this_tmp= {};
                        std::insert_iterator<std::set<Instruction*,cmp_expr>> it(this_tmp,this_tmp.begin());
                        std::set_intersection(last_tmp.begin(),last_tmp.end(),bb_out[pred].begin(),bb_out[pred].end(),it);
                        last_tmp = this_tmp;
                    }else{
                        is_first = false;
                        last_tmp = bb_out[pred];
                    }
                }
                bb_in[bb] = last_tmp;
                auto old_out_size = bb_out[bb].size();
                std::set<Instruction*,cmp_expr> tmp2 = {};
                std::insert_iterator<std::set<Instruction*,cmp_expr>> it(tmp2,tmp2.begin());
                //std::set_difference(bb_in[bb].begin(),bb_in[bb].end(),bb_kill[bb].begin(),bb_kill[bb].end(),it);
                //std::set<Instruction*,cmp_expr> tmp3 = {};
                //std::insert_iterator<std::set<Instruction*,cmp_expr>> it2(tmp3,tmp3.begin());
                std::set_union(bb_gen[bb].begin(),bb_gen[bb].end(),bb_in[bb].begin(),bb_in[bb].end(),it);
                bb_out[bb] = tmp2;
                auto new_out_size = tmp2.size();
                if(old_out_size!=new_out_size){
                    change = true;
                }
            }
        }
    }
}

void AvailableExpr::initial_map(Function *f) {
    auto all_bbs = f->get_basic_blocks();
    for(auto bb:all_bbs){
        bb_in[bb] = std::set<Instruction*,cmp_expr>();
        bb_out[bb] = std::set<Instruction*,cmp_expr>();
        bb_gen[bb] = std::set<Instruction*,cmp_expr>();
        //bb_kill[bb] = std::set<Instruction*,cmp_expr>();
    }
}

//void AvailableExpr::remove_relevant_instr(Value *val, std::set<Instruction *, cmp_expr> &bb_set) {
//    for(auto instr_iter = bb_set.begin();instr_iter!=bb_set.end();instr_iter++){
//        auto cur_instr = *instr_iter;
//        for(auto opr:cur_instr->get_operands()){
//            if(opr==val){
//                instr_iter = bb_set.erase(instr_iter);
//                instr_iter --;
//                break;
//            }
//        }
//    }
//}

//void AvailableExpr::insert_relevant_instr(Value *val, std::set<Instruction *, cmp_expr> &bb_set) {
//    for(auto & map_iter : bb_gen){
//        auto cur_gen = map_iter.second;
//        for(auto instr:cur_gen){
//            for(auto opr:instr->get_operands()){
//                if(opr==val){
//                    bb_set.insert(instr);
//                    break;
//                }
//            }
//        }
//    }
//}

void AvailableExpr::compute_global_common_expr(Function *f) {
    auto all_bbs = f->get_basic_blocks();
    for(auto bb:all_bbs){
        auto instrs = bb->get_instructions();
        for(auto instr_iter=instrs.begin();instr_iter!=instrs.end();instr_iter++){
            auto instr = *instr_iter;
            auto common_exp = bb_in[bb].find(instr);
            if(common_exp!=bb_in[bb].end()){
                instr->replace_all_use_with(*common_exp);
                instr_iter = instrs.erase(instr_iter);
                instr_iter --;
                instr->remove_use_of_ops();
            }
        }
    }
}

