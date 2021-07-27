//
// Created by cjb on 7/24/21.
//

#include "RegAlloc.h"
#include <set>

void Interval::add_range(int from, int to) {
    auto top_range = *range_list.begin();
    if(from>=top_range->from && from<=top_range->to){
        top_range->to = to > top_range->to?to:top_range->to;
    }else if(from < top_range->from){
        if(to <= top_range->to && to>=top_range->from){
            top_range->from = from;
        }else{
            auto new_range = new Range(from,to);
            range_list.push_front(new_range);
        }
    }else{
        auto new_range = new Range(from,to);
        range_list.push_front(new_range);
    }
}


bool Interval::intersects(Interval *interval) {
    auto taget_it = range_list.begin();
    auto with_it = interval->range_list.begin();
    while(with_it!=interval->range_list.end()&&taget_it!=range_list.end()){
        auto target_range = *taget_it;
        auto with_range = *with_it;
        if(target_range->to<=with_range->from){
            taget_it++;
            continue;
        }else if(with_range->to<=target_range->from){
            with_it++;
            continue;
        }else{
            return true;
        }
    }
    return false;
}


struct cmp_range{
    bool operator()(Range* a,Range* b){
        return a->from > b->from;
    }
};

void Interval::union_interval(Interval *interval) {
    std::priority_queue<Range*, std::list<Range*>, cmp_range> all_range;
    for(auto range:range_list){
        all_range.push(range);
    }
    for(auto range:interval->range_list){
        all_range.push(range);
    }
    if(all_range.empty()){
        return;
    }
    range_list.clear();
    auto cur_range = all_range.top();
    all_range.pop();
    while(!all_range.empty()){
        auto merge_range = all_range.top();
        all_range.pop();
        if(merge_range->from > cur_range->to){
            range_list.push_back(cur_range);
            cur_range = merge_range;
        }else{
            cur_range->to = cur_range->to >= merge_range->to?cur_range->to:merge_range->to;
        }
    }
    range_list.push_back(cur_range);
}


void RegAllocDriver::compute_reg_alloc() {
    for(auto func:module->get_functions()){
        if(func->get_basic_blocks().empty()){
            continue;
        }else{
            auto allocator = new RegAlloc(func);
            allocator->execute();
            reg_alloc[func] = allocator->get_reg_alloc();
        }
    }
}

void RegAlloc::execute() {
    compute_block_order();
    number_operations();
    build_intervals();
    union_phi_val();
    walk_intervals();
}

struct cmp_block_depth{
    bool operator()(BasicBlock* a,BasicBlock* b){
        return a->get_loop_depth() < b->get_loop_depth();
    }
};

//ref: https://ssw.jku.at/Research/Papers/Wimmer04Master/Wimmer04Master.pdf
void RegAlloc::compute_block_order() {
//TODO:USE LOOP INFO
//TODO:CHECK CLEAR
    std::priority_queue<BasicBlock*,std::list<BasicBlock*>,cmp_block_depth>work_list;
    block_order.clear();
    auto entry = func->get_entry_block();
    work_list.push(entry);
    while(!work_list.empty()){
        auto bb = work_list.top();
        work_list.pop();
        block_order.push_back(bb);

        for(auto sux : bb->get_succ_basic_blocks()){
            sux->incoming_decrement();
            if(sux->is_incoming_zero()){
                work_list.push(sux);
            }
        }
    }
//    std::set<BasicBlock*> visited = {};
//    get_dfs_order(entry,visited);
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
            if(!dynamic_cast<Instruction*>(opr)){
                continue;
            }
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
            //TODO:ADD FUN CALL?

//            if(instr->is_call()){
//                //TODO:ADD PHYSICAL REG?
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
                if(!dynamic_cast<Instruction*>(opr)){
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
        auto current_it = interval_list.begin();
        current = *current_it;//TODO:CHECK WARNING
        interval_list.erase(current_it);
        //interval_list.pop();
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
    auto vreg_sets = func->get_vreg_set();
    for(auto set:vreg_sets){
        Value* final_vreg = nullptr;
        for(auto vreg:set){
            if(final_vreg == nullptr){
                final_vreg = vreg;
            }else{
                auto vreg_ptr = val2Inter[vreg];
                auto final_ptr = val2Inter[final_vreg];
                final_ptr->union_interval(vreg_ptr);
                val2Inter[vreg] = final_ptr;
                interval_list.erase(vreg_ptr);
            }
        }
    }
}