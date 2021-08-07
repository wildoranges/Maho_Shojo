#include "DeadCodeElimination.h"

#include <algorithm>

void DeadCodeElimination::execute() {
    for (auto func : module->get_functions()) {
        if (func->get_num_basic_blocks() == 0) {
            continue;
        }
        func_ = func;
        instr_mark.clear();
        RDominateTree r_dom_tree(module);
        r_dom_tree.execute();
        mark();
        remove_unmarked_bb();
        RDominateTree r_dom_tree_new(module);
        r_dom_tree_new.execute();
        sweep();
    }
    return ;
}

bool DeadCodeElimination::is_critical(Instruction *instr) {
    if (instr->is_ret()) return true;
    else if (instr->is_call()) return true;
    else if (instr->is_store() || instr->is_store_offset()) {
        auto addr = instr->get_operand(1);
        auto alloca_addr = dynamic_cast<AllocaInst*>(addr);
        auto global_addr = dynamic_cast<GlobalVariable*>(addr);
        auto arg_addr = dynamic_cast<Argument*>(addr);
        if ((arg_addr && arg_addr->get_type()->is_integer_type()) ||
            (alloca_addr && alloca_addr->get_alloca_type() == Type::get_int32_type(module)) ||
            (global_addr && global_addr->get_type()->is_pointer_type()) && global_addr->get_type()->get_pointer_element_type()->is_integer_type()) {
            for (auto use : addr->get_use_list()) {
                auto use_instr = dynamic_cast<Instruction*>(use.val_);
                if (use_instr->is_load()) {
                    return true;
                }
            }
            return false;
        }
        return true;
    }
    else return false;
}

BasicBlock *DeadCodeElimination::get_nearest_marked_postdominator(Instruction *instr) {
    std::list<BasicBlock*> visit_bb;
    std::map<BasicBlock*, bool> visited_bb_map;
    auto instr_bb = instr->get_parent();
    auto instr_bb_rdoms = instr_bb->get_rdoms();
    for (auto bb : instr_bb_rdoms) {
        visited_bb_map.insert({bb, false});
    }
    visit_bb.push_back(instr_bb);
    while (visit_bb.empty() == false) {
        auto curr_bb = visit_bb.front();
        auto next_bb_list = curr_bb->get_succ_basic_blocks();
        for (auto next_bb : next_bb_list) {
            if (next_bb != instr_bb && instr_bb_rdoms.find(next_bb) != instr_bb_rdoms.end()) {
                if (instr_mark[next_bb->get_terminator()] == true) {
                    return next_bb;
                }
            }
            if (visited_bb_map[next_bb] == false) {
                visit_bb.push_back(next_bb);
                visited_bb_map[next_bb] = true;
            }
        }
        visit_bb.pop_front();
    }
    return nullptr;
}

void DeadCodeElimination::remove_unmarked_bb() {
    std::vector<BasicBlock*> wait_delete_bb;
    for (auto bb : func_->get_basic_blocks()) {
        if (bb == func_->get_entry_block()) continue;
        bool marked = false;
        for (auto instr : bb->get_instructions()) {
            if (instr_mark[instr] == true) {
                marked = true;
                break;
            }
        }
        if (marked == false) {
            wait_delete_bb.push_back(bb);
        }
    }
    for (auto delete_bb : wait_delete_bb) {
        func_->remove(delete_bb);
    }
}

void DeadCodeElimination::mark() {
    std::vector<Instruction *> work_list;
    for (auto bb : func_->get_basic_blocks()) {
        for (auto instr : bb->get_instructions()) {
            if (is_critical(instr)) {
                instr_mark.insert({instr, true});
                work_list.push_back(instr);
            } else {
                instr_mark.insert({instr, false});
            }
        }
    }
    while (work_list.empty() == false) {
        auto instr = work_list.back();
        work_list.pop_back();
        for (auto operand : instr->get_operands()) {
            auto def = dynamic_cast<Instruction*>(operand);
            auto bb = dynamic_cast<BasicBlock*>(operand);
            if (def != nullptr && instr_mark[def] == false) {
                instr_mark[def] = true;
                work_list.push_back(def);
            }
            if (bb != nullptr) {
                auto bb_terminator = bb->get_terminator();
                if (instr_mark[bb_terminator] == false) {
                    instr_mark[bb_terminator] = true;
                    work_list.push_back(bb_terminator);
                }
            }
        }
        for (auto reverse_dom_froniter_bb : instr->get_parent()->get_rdom_frontier()) {
            auto terminator = reverse_dom_froniter_bb->get_terminator();
            if ((terminator->is_br() || terminator->is_cmpbr()) && instr_mark[terminator] == false) {
                instr_mark[terminator] = true;
                work_list.push_back(terminator);
            }
        }
    }
    return ;
}

void DeadCodeElimination::sweep() {
    std::vector<Instruction*> wait_delete;
    for (auto bb : func_->get_basic_blocks()) {
        for (auto instr : bb->get_instructions()) {
            if (instr_mark[instr] == false) {
                if (instr->is_br() && instr->get_num_operand() == 3) {
                    auto targetBB = get_nearest_marked_postdominator(instr);
                    instr->remove_use_of_ops();
                    instr->set_operand(0, targetBB);
                } else if (instr->is_cmpbr() && instr->get_num_operand() == 4) {
                    auto targetBB = get_nearest_marked_postdominator(instr);
                    auto true_bb = dynamic_cast<BasicBlock* >(instr->get_operand(2));
                    auto false_bb = dynamic_cast<BasicBlock* >(instr->get_operand(3));
                    true_bb->remove_pre_basic_block(bb);
                    false_bb->remove_pre_basic_block(bb);
                    bb->remove_succ_basic_block(true_bb);
                    bb->remove_succ_basic_block(false_bb);;
                    BranchInst::create_br(targetBB, bb);
                }
                if (!(instr->is_br() && instr->get_num_operand() == 1)) {
                    wait_delete.push_back(instr);
                }
            }
        }
        for (auto instr : wait_delete) {
            //std::cerr<<"delete "<<instr->print()<<std::endl;
            // TODO: 绑定成一条汇编指令的ir指令集合(如smul_lo和smul_hi绑定为smul)要么全部传播,要么都不传播
            if (instr->is_smul_lo() || instr->is_smul_hi()) {
                continue;
            }
            bb->delete_instr(instr);
        }
    }
    return ;
}
