#include "DeadCodeElimination.h"

#include <algorithm>

void DeadCodeElimination::execute() {
    for (auto func : module->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            bb_ = bb;
            mark();
            sweep();
        }
    }
    return ;
}

bool DeadCodeElimination::is_critical(Instruction *instr) {
    return (instr->is_ret() && instr->get_parent()->get_parent()->get_return_type()->is_void_type() == false);
}

BasicBlock *DeadCodeElimination::get_nearest_marked_postdominator(Instruction *instr) {
    std::list<BasicBlock*> visit_bb;
    std::map<BasicBlock*, bool> visited_bb_map;
    auto instr_bb = instr->get_parent();
    auto instr_bb_rdoms = instr_bb->get_rdoms();
    std::cout<<instr_bb->get_parent()->get_name()<<std::endl;
    std::cout<<instr_bb_rdoms.size()<<std::endl;
    for (auto bb : instr_bb_rdoms) {
        visited_bb_map.insert({bb, false});
    }
    visit_bb.push_back(instr_bb);
    while (visit_bb.empty() == false) {
        auto curr_bb = visit_bb.front();
        auto next_bb_list = curr_bb->get_succ_basic_blocks();
        for (auto next_bb : next_bb_list) {
            if (next_bb != instr_bb && instr_bb_rdoms.find(next_bb) != instr_bb_rdoms.end()) {
                std::cout<<next_bb->print()<<std::endl;
                std::cout<<next_bb->get_terminator()->print()<<std::endl;
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

void DeadCodeElimination::mark() {
    std::vector<Instruction *> work_list;
    instr_mark.clear();
    for (auto instr : bb_->get_instructions()) {
        if (is_critical(instr)) {
            instr_mark.insert({instr, true});
            work_list.push_back(instr);
        } else {
            instr_mark.insert({instr, false});
        }
    }
    while (work_list.empty() == false) {
        auto instr = work_list.back();
        work_list.pop_back();
        if (instr->get_num_operand() == 2 || ((instr->is_br() || instr->is_cmpbr()) && instr->get_num_operand() > 1)) {
            auto def_1 = dynamic_cast<Instruction*>(instr->get_operand(0));
            auto def_2 = dynamic_cast<Instruction*>(instr->get_operand(1));
            if (def_1 != nullptr && instr_mark[def_1] == false) {
                instr_mark[def_1] = true;
                work_list.push_back(def_1);
            }
            if (def_2 != nullptr && instr_mark[def_2] == false) {
                instr_mark[def_2] = true;
                work_list.push_back(def_2);
            }
            for (auto reverse_dom_froniter_bb : bb_->get_rdom_frontier()) {
                auto terminator = reverse_dom_froniter_bb->get_terminator();
                if ((terminator->is_br() || terminator->is_cmpbr()) && instr_mark[terminator] == false) {
                    instr_mark[terminator] = true;
                    work_list.push_back(terminator);
                }
            }
        }
    }
    return ;
}

void DeadCodeElimination::sweep() {
    std::vector<Instruction*> wait_delete;
    for (auto instr : bb_->get_instructions()) {
        if (instr_mark[instr] == false) {
            if (instr->is_br() && instr->get_num_operand() == 3) {
                auto targetBB = get_nearest_marked_postdominator(instr);
                instr->remove_use_of_ops();
                instr->set_operand(0, targetBB);
            } else if (instr->is_cmpbr() && instr->get_num_operand() == 4) {
                auto targetBB = get_nearest_marked_postdominator(instr);
                auto true_bb = dynamic_cast<BasicBlock* >(instr->get_operand(2));
                auto false_bb = dynamic_cast<BasicBlock* >(instr->get_operand(3));
                true_bb->remove_pre_basic_block(bb_);
                false_bb->remove_pre_basic_block(bb_);
                bb_->remove_succ_basic_block(true_bb);
                bb_->remove_succ_basic_block(false_bb);;
                auto new_br = BranchInst::create_br(targetBB, bb_);
            }
            if (!(instr->is_br() && instr->get_num_operand() == 1)) {
                wait_delete.push_back(instr);
            }
        }
    }
    for (auto instr : wait_delete) {
        bb_->delete_instr(instr);
    }
    return ;
}
