#include "ActiveVar.h"

#include <algorithm>

void ActiveVar::execute() {
    for (auto &func : this->module->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            func_ = func;  
            live_in.clear();
            live_out.clear();
            def_var.clear();
            use_var.clear();
            get_def_var();
            get_use_var();
            get_live_in_live_out();
            for (auto bb : func_->get_basic_blocks()) {
                bb->set_use_var(use_var[bb]);
                bb->set_def_var(def_var[bb]);
                bb->set_live_in(live_in[bb]);
                bb->set_live_out(live_out[bb]);
            }
        }
    }
    return ;
}

void ActiveVar::get_def_var() {
    for (auto bb : func_->get_basic_blocks()) {
        def_var.insert({bb, {}});
        for (auto instr : bb->get_instructions()) {
            if (instr->is_void()) {
                continue;
            }
            def_var[bb].insert(instr);
        }
    }
    return ;
}

void ActiveVar::get_use_var() {
    for (auto bb : func_->get_basic_blocks()) {
        use_var.insert({bb, {}});
        for (auto instr : bb->get_instructions()) {
            for (auto op: instr->get_operands()) {
                if (dynamic_cast<ConstantInt*>(op)) continue;
                if (dynamic_cast<BasicBlock*>(op)) continue;
                if (dynamic_cast<Function*>(op)) continue;
                use_var[bb].insert(op);
            }
        }
        for (auto var : def_var[bb]) {
            if (use_var[bb].find(var) != use_var[bb].end()) {
                use_var[bb].erase(var);
            }
        }
    }
    return ;
}

void ActiveVar::get_live_in_live_out() {
    get_def_var();
    get_use_var();
    for (auto bb : func_->get_basic_blocks()) {
        live_in.insert({bb, {}});
        live_out.insert({bb, {}});
    }
    bool repeat = true;
    while (repeat) {
        repeat = false;
        for (auto bb : func_->get_basic_blocks()) {
            std::set<Value *> tmp_live_out = {};
            for (auto succBB : bb->get_succ_basic_blocks()) {
                auto succ_tmp_live_in = live_in[succBB];
                std::set<Value *> active_val = {};
                for (auto instr : succBB->get_instructions()) {
                    if (instr->is_phi()) {
                        for (int i = 1; i < instr->get_num_operand(); i+=2) {
                            if (instr->get_operand(i) == bb) {
                                if (active_val.find(instr->get_operand(i - 1)) == active_val.end()) {
                                    active_val.insert(instr->get_operand(i - 1));
                                }
                            }
                        }
                    } else {
                        for (auto op: instr->get_operands()) {
                            if (dynamic_cast<ConstantInt*>(op)) continue;
                            if (dynamic_cast<BasicBlock*>(op)) continue;
                            if (dynamic_cast<Function*>(op)) continue;
                            active_val.insert(op);
                        }
                    }
                }
                for (auto var : def_var[succBB]) {
                    if (active_val.find(var) != active_val.end()) {
                        active_val.erase(var);
                    }
                }
                for (auto instr : succBB->get_instructions()) {
                    if (instr->is_phi()) {
                        for (int i = 1; i < instr->get_num_operand(); i+=2) {
                            if (instr->get_operand(i) != bb) {
                                if (succ_tmp_live_in.find(instr->get_operand(i - 1)) != succ_tmp_live_in.end()) {
                                    succ_tmp_live_in.erase(instr->get_operand(i - 1));
                                }
                            }
                        }
                    }
                }
                std::set_union(succ_tmp_live_in.begin(), succ_tmp_live_in.end(), active_val.begin(), active_val.end(), std::inserter(succ_tmp_live_in, succ_tmp_live_in.begin()));
                std::set_union(tmp_live_out.begin(), tmp_live_out.end(), succ_tmp_live_in.begin(), succ_tmp_live_in.end(), std::inserter(tmp_live_out, tmp_live_out.begin()));
            }
            // 迭代后的in和out必不可能小于迭代前的in和out(归纳法可证)
            std::set_union(live_out[bb].begin(), live_out[bb].end(), tmp_live_out.begin(), tmp_live_out.end(), std::inserter(live_out[bb], live_out[bb].begin()));
            auto tmp_live_in = tmp_live_out;
            std::set<Value *> tmp;
            std::set_difference(tmp_live_in.begin(), tmp_live_in.end(), def_var[bb].begin(), def_var[bb].end(), std::inserter(tmp, tmp.begin()));
            tmp_live_in = tmp;
            std::set_union(tmp_live_in.begin(), tmp_live_in.end(), use_var[bb].begin(), use_var[bb].end(), std::inserter(tmp_live_in, tmp_live_in.begin()));
            auto old_live_in_size = live_in[bb].size();
            std::set_union(live_in[bb].begin(), live_in[bb].end(), tmp_live_in.begin(), tmp_live_in.end(), std::inserter(live_in[bb], live_in[bb].begin()));
            if (live_in[bb].size() > old_live_in_size) {
                repeat = true;
            }
        }
    }
    return ;
}
