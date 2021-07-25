//
// Created by cjb on 7/24/21.
//

#include "RegAlloc.h"
#include <set>

void Interval::add_range(int from, int to) {

}


void Interval::add_use_pos(int pos) {

}


bool Interval::intersects(Interval *interval) {

}


void Interval::union_interval(Interval *interval) {

}

//ref: https://ssw.jku.at/Research/Papers/Wimmer04Master/Wimmer04Master.pdf
void RegAlloc::compute_block_order() {
//TODO:USE LOOP INFO
//TODO:CHECK CLEAR
    block_order.clear();
    auto entry = func->get_entry_block();
    std::set<BasicBlock*> visited = {};
    get_dfs_order(entry,visited);
}

void RegAlloc::get_dfs_order(BasicBlock *bb, std::set<BasicBlock *> &visited) {
    visited.insert(bb);
    block_order.push_back(bb);
    auto children = bb->get_succ_basic_blocks();
    for(auto child : children){
        auto is_visited = visited.find(child);
        if(is_visited == visited.end()){
            get_dfs_order(child,visited);
        }
    }
}

void RegAlloc::number_operations() {
    int next_id = 0;
    for(auto bb:block_order){
        auto instrs = bb->get_instructions();
        for(auto instr:instrs){
            instr->set_id(next_id);
            next_id += 2;
        }
    }
}

void RegAlloc::build_intervals() {//TODO:CHECK EMPTY BLOCK
    for(auto iter = block_order.rbegin();iter != block_order.rend();iter++)
    {
        //TODO:CONST CHECK
        auto bb = *iter;
        auto instrs = bb->get_instructions();
        int block_from = (*(instrs.begin()))->get_id();
        auto lst_instr = instrs.rbegin();
        int block_to = (*(lst_instr))->get_id() + 2;
        for(auto opr:bb->get_live_out()){//TODO:NEW
            if(val2Inter.find(opr)==val2Inter.end()){
                auto new_interval = new Interval(opr);
                val2Inter[opr] = new_interval;
                add_interval(new_interval);
            }
            val2Inter[opr]->add_range(block_from,block_to);
        }
        for(auto instr_iter = instrs.rbegin();instr_iter!=instrs.rend();instr_iter++){
            auto instr = *instr_iter;
//            if(instr->is_phi()){
//                continue;
//            }//TODO:SAME COLOR OF PHI?
            //TODO:ADD FUN CALL

//            if(instr->is_call()){
//                //TODO:ADD PHYSICAL REG
//            }

            if(!instr->is_void()){
                if(val2Inter.find(instr)==val2Inter.end()){//TODO:MUST BE NOT NULL?
                    auto new_interval = new Interval(instr);
                    new_interval->add_range(block_from,block_to);
                    val2Inter[instr] = new_interval;
                    add_interval(new_interval);
                }
                auto top_range = *val2Inter[instr]->range_list.begin();
                //val2Inter[instr]->range_list.pop();
                top_range->from = instr->get_id();
                //val2Inter[instr]->range_list.push(top_range);
                val2Inter[instr]->add_use_pos(instr->get_id());
            }

            for(auto opr:instr->get_operands()){
                if(dynamic_cast<Constant*>(opr)){
                    continue;
                }
                if(val2Inter.find(opr)==val2Inter.end()){
                    auto new_interval = new Interval(opr);
                    val2Inter[opr] = new_interval;
                    add_interval(new_interval);
                }
                val2Inter[opr]->add_range(block_from,instr->get_id());
                val2Inter[opr]->add_use_pos(instr->get_id());
            }
        }
    }
}

void RegAlloc::walk_intervals() {
    active = {};
    //inactive = {};
    //handled = {};
    while(!interval_list.empty()){
        current = interval_list.top();
        interval_list.pop();
        auto position = (*current->range_list.begin())->from;

        for(auto it = active.begin();it != active.end();it++){
            if((*((*it)->range_list.rbegin()))->to < position){//TODO:CHECK equal?
                add_reg_to_pool((*it)->reg_num);
                //handled.insert(*it);
                it = active.erase(it);
                it--;
            }
        }

//        for(auto it = inactive.begin();it != inactive.end();it++){
//            if((*((*it)->range_list.rbegin()))->to < position){
//                handled.insert(*it);
//                it = inactive.erase(it);
//                it--;
//            }else if((*it)->covers(position)){
//                active.insert(*it);
//                it = inactive.erase(it);
//                it--;
//            }
//        }

        if(try_alloc_free_reg()){//for debug
            std::cout << "alloc reg " << current->reg_num << " for val "<<current->val->get_name()<<std::endl;
        }else{
            std::cout << "spill to stack for val "<<current->val->get_name()<<std::endl;
        }
    }
}

bool RegAlloc::try_alloc_free_reg() {
    if(!remained_general_reg_id.empty()){
        int assigned_id = remained_general_reg_id.top();
        remained_general_reg_id.pop();
        current->reg_num = assigned_id;
        active.insert(current);
        return true;
    }else if(!remained_func_reg_id.empty()){
        int assigned_id = remained_func_reg_id.top();
        remained_func_reg_id.pop();
        current->reg_num = assigned_id;
        active.insert(current);
        return true;
    }else{
        auto spill_val = current;
        int max_expire_pos = (*(current->range_list.rbegin()))->to;
        for(auto it:active){
            int cur_expire_pos = (*(it->range_list.rbegin()))->to;
            if(cur_expire_pos > max_expire_pos){
                max_expire_pos = cur_expire_pos;
                spill_val = it;
            }
            if(!it->intersects(current)){
                current->reg_num = it->reg_num;
                active.insert(current);
                return true;
            }
        }
        if(spill_val==current){
            current->reg_num = -1;
            return false;
        }else{
            current->reg_num = spill_val->reg_num;
            spill_val->reg_num = -1;
            active.insert(current);
            active.erase(spill_val);//TODO:CHECK ERASE?
            return true;
        }
    }
}

void RegAlloc::add_reg_to_pool(int reg_id) {
    if(reg_id >= 4&&reg_id<=12){
        remained_general_reg_id.push(reg_id);
    }else if(reg_id >= 0&&reg_id <= 3){
        remained_func_reg_id.push(reg_id);
    }
}

void RegAlloc::union_phi_val() {

}