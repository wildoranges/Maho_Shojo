#include "SCCP.h"

void SCCP::execute() {
    for (auto func : module->get_functions()) {
        func_ = func;
        CFGWorkList.clear();
        SSAWorkList.clear();
        CFGWorkList_mark.clear();
        lattice_value_map.clear();
        ActiveVar active_var(module);
        active_var.execute();
        for (auto bb : func->get_basic_blocks()) {
            for (auto succ_bb : bb->get_succ_basic_blocks()) {
                if (bb == func->get_entry_block()) {
                    CFGWorkList.push_back({bb, succ_bb});
                }
                CFGWorkList_mark.insert({{bb, succ_bb}, unexecuted});
            }
            for (auto def : bb->get_def_var()) {
                lattice_value_map.insert({def, top});
            }
            for (auto use : bb->get_use_var()) {
                lattice_value_map.insert({use, top});
            }
        }
        while (CFGWorkList.empty() == false || SSAWorkList.empty() == false) {
            if (CFGWorkList.empty() == false) {
                auto edge = CFGWorkList.back();
                CFGWorkList.pop_back();
                if (CFGWorkList_mark[edge] == unexecuted) {
                    CFGWorkList_mark[edge] = executed;
                    evaluate_all_phis_in_block(edge);
                    auto succ_bb = edge.second;
                    auto executed_flag = false;
                    for (auto pre_bb : succ_bb->get_pre_basic_blocks()) {
                        if (pre_bb == edge.first) continue;
                        if (CFGWorkList_mark[{pre_bb, succ_bb}] == executed) {
                            executed_flag = true;
                            break;
                        }
                    }
                    if (executed_flag == false) {
                        if ()
                    }
                }
            }
        }
    }
}

void SCCP::evaluate_assign() {
    
}

void SCCP::evaluate_conditional() {
    
}

void SCCP::evaluate_phi() {
    
}

void SCCP::evaluate_all_phis_in_block(std::pair<BasicBlock*, BasicBlock*> edge) {
    
}

void SCCP::evaluate_operands() {
    
}

void SCCP::evaluate_result() {
    
}
