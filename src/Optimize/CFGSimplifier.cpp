#include "CFGSimplifier.h"

void CFGSimplifier::execute() {
    for (auto func : module->get_functions()) {
        if (func->get_num_basic_blocks() == 0) {
            continue;
        }
        func_ = func;
        postorder_bb_list.clear();
        bool changed = true;
        while (changed) {
            postorder_bb_list.clear();
            compute_postorder();
            changed = one_pass();
        }
    }
    return ;
}

void CFGSimplifier::compute_postorder() {
    std::map<BasicBlock*, bool> visited_bb;
    std::vector<BasicBlock*> dfs_bb_list;
    auto bb_list = func_->get_basic_blocks();
    for (auto bb : bb_list) {
        visited_bb.insert({bb, false});
    }
    dfs_bb_list.push_back(func_->get_entry_block());
    while (dfs_bb_list.empty() == false) {
        auto cur_bb = dfs_bb_list.back();
        bool flag = false;
        for (auto next_bb : cur_bb->get_succ_basic_blocks()) {
            if (visited_bb[next_bb] == false) {
                visited_bb[next_bb] = true;
                dfs_bb_list.push_back(next_bb);
                flag = true;
                break;
            }
        }
        if (flag == true) {
            continue;
        } else {
            postorder_bb_list.push_back(cur_bb);
            dfs_bb_list.pop_back();
        }
    }
    return ;
}

void CFGSimplifier::combine_bb(BasicBlock *bb, BasicBlock *succ_bb) {
    // bb has only one successor succ_bb and succ_bb has only one predecessor bb
    bb->remove_succ_basic_block(succ_bb);
    succ_bb->remove_pre_basic_block(bb);
    for (auto succ_succ_bb : succ_bb->get_succ_basic_blocks()) {
        succ_succ_bb->remove_pre_basic_block(succ_bb);
        bb->add_succ_basic_block(succ_succ_bb);
        succ_succ_bb->add_pre_basic_block(bb);
        for (auto instr : succ_succ_bb->get_instructions()) {
            if (instr->is_phi()) {
                for (int i = 1; i < instr->get_num_operand(); i+=2) {
                    auto target_bb = dynamic_cast<BasicBlock*>(instr->get_operand(i));
                    if (target_bb != nullptr && target_bb == succ_bb) {
                        instr->set_operand(i, bb);
                    }
                }
            }
        }
    }
    bb->delete_instr(bb->get_terminator());
    for (auto instr : succ_bb->get_instructions()) {
        if (instr->is_phi()) {
            bb->add_instr_begin(instr);
        } else {
            bb->add_instruction(instr);
        }
    }
    return ;
}

bool CFGSimplifier::one_pass() {
    std::vector<BasicBlock*> wait_delete_bb;
    bool changed = false;
    for (auto bb : postorder_bb_list) {
        if (bb == func_->get_entry_block()) {
            continue;
        }
        auto terminator = bb->get_terminator();
        if (terminator->is_br() && terminator->get_num_operand() == 3) {
            if (terminator->is_br()) {
                auto true_bb = dynamic_cast<BasicBlock*>(terminator->get_operand(1));
                auto false_bb = dynamic_cast<BasicBlock*>(terminator->get_operand(2));
                if (true_bb != nullptr && false_bb != nullptr && true_bb == false_bb) {
                    terminator->remove_use_of_ops();
                    terminator->set_operand(0, true_bb);
                    changed = true;
                }
            }
        }
        if (terminator->is_br() && terminator->get_num_operand() == 1) {
            auto succ_bb = dynamic_cast<BasicBlock*>(terminator->get_operand(0));
            auto succ_bb_terminator = dynamic_cast<Instruction*>(succ_bb->get_terminator());
            if (bb->get_num_of_instr() == 1) {  // empty bb
                for (auto pre_bb : bb->get_pre_basic_blocks()) {
                    pre_bb->remove_succ_basic_block(bb);
                    pre_bb->add_succ_basic_block(succ_bb);
                    succ_bb->add_pre_basic_block(pre_bb);
                    auto pre_terminator = pre_bb->get_terminator();
                    for (int i = 0; i < pre_terminator->get_num_operand() ; i++) {
                        auto target_bb = dynamic_cast<BasicBlock*>(pre_terminator->get_operand(i));
                        if (target_bb == bb) {
                            pre_terminator->set_operand(i, succ_bb);
                        }
                    }
                    for (auto instr : succ_bb->get_instructions()) {
                        if (instr->is_phi()) {
                            for (int i = 1; i < instr->get_num_operand(); i+=2) {
                                auto target_bb = dynamic_cast<BasicBlock*>(instr->get_operand(i));
                                if (target_bb != nullptr && target_bb == bb) {
                                    instr->set_operand(i, pre_bb);
                                }
                            }
                        }
                    }
                }
                wait_delete_bb.push_back(bb);
                changed = true;
            }
            if (succ_bb->get_pre_basic_blocks().size() == 1) {  // succ_bb only has one predecessor, combine them
                combine_bb(bb, succ_bb);
                wait_delete_bb.push_back(succ_bb);
                changed = true;
            }
            if (succ_bb->get_num_of_instr() == 1 && succ_bb_terminator->is_br() && succ_bb_terminator->get_num_operand() > 1) {
                bb->delete_instr(terminator);
                bb->add_instruction(succ_bb_terminator);
                for (auto succ_succ_bb : succ_bb->get_succ_basic_blocks()) {
                    succ_succ_bb->remove_pre_basic_block(succ_bb);
                    bb->add_succ_basic_block(succ_succ_bb);
                    succ_succ_bb->add_pre_basic_block(bb);
                    for (auto instr : succ_succ_bb->get_instructions()) {
                        if (instr->is_phi()) {
                            for (int i = 1; i < instr->get_num_operand(); i+=2) {
                                auto target_bb = dynamic_cast<BasicBlock*>(instr->get_operand(i));
                                if (target_bb != nullptr && target_bb == succ_bb) {
                                    instr->set_operand(i, bb);
                                }
                            }
                        }
                    }
                }
                changed = true;
            }
        }
    }
    for (auto delete_bb : wait_delete_bb) {
        for (auto bb : func_->get_basic_blocks()) {
            for (auto instr : bb->get_instructions()) {
                if (instr->is_phi()) {
                    for (int i = 1; i < instr->get_num_operand(); i+=2) {
                        if (instr->get_operand(i) == delete_bb) {
                            instr->remove_operands(i - 1, i);
                        }
                    }
                }
            }
        }
        func_->remove(delete_bb);
    }
    return changed;
}
