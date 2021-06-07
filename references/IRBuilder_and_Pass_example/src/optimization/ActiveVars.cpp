#include "ActiveVars.hpp"

void ActiveVars::get_def_vars(Function *fun)
{
    for (auto bb : fun->get_basic_blocks()) {
        def_vars.insert({bb, {}});
        for (auto instr : bb->get_instructions()) {
            if (instr->is_ret() || instr->is_store()) {
                continue;
            }
            def_vars[bb].insert(instr);
        }
    }
    return ;
}

void ActiveVars::get_use_vars(Function *fun)
{
    for (auto bb : fun->get_basic_blocks()) {
        use_vars.insert({bb, {}});
        for (auto instr : bb->get_instructions()) {
            if (instr->is_alloca()) {
                continue;
            }
            else if (instr->is_br()) {
                if (instr->get_num_operand() == 3) { // 条件跳转
                    auto operand = instr->get_operand(0);
                    if (cast_constantint(operand) || cast_constantfp(operand)) {
                        continue;
                    }
                    else {
                        use_vars[bb].insert(operand);
                    }
                }
            }
            else if (instr->is_phi()) {
                for (int i = 0; i < instr->get_num_operand(); i+=2) {
                    auto operand = instr->get_operand(i);
                    if (cast_constantint(operand) || cast_constantfp(operand)) {
                        continue;
                    }
                    else {
                        use_vars[bb].insert(operand);
                    }
                }
            }
            else {
                for (auto operand : instr->get_operands()) {
                    if (instr->is_call()) {
                        if (operand == instr->get_operand(0)) { // 函数名
                            continue;
                        }
                    }
                    if (cast_constantint(operand) || cast_constantfp(operand)) {
                        continue;
                    }
                    else {
                        use_vars[bb].insert(operand);
                    }
                }
            }
        }
        for (auto var : def_vars[bb]) {
            if (use_vars[bb].find(var) != use_vars[bb].end()) {
                use_vars[bb].erase(var);
            }
        }
    }
    return ;
}

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内
            def_vars.clear();
            use_vars.clear();
            get_def_vars(func_);
            get_use_vars(func_);
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
                        for (auto var : succ_tmp_live_in) { // 取并集
                            if (tmp_live_out.find(var) == tmp_live_out.end()) {
                                tmp_live_out.insert(var);
                            }
                        }
                    }
                    for (auto var : tmp_live_out) { // 迭代后的in和out必不可能小于迭代前的in和out(归纳法可证)
                        if (live_out[bb].find(var) == live_out[bb].end()) {
                            live_out[bb].insert(var);
                        }
                    }
                    auto tmp_live_in = tmp_live_out;
                    for (auto def_var : def_vars[bb]) {
                        if (tmp_live_in.find(def_var) != tmp_live_in.end()) {
                            tmp_live_in.erase(def_var);
                        }
                    }
                    for (auto use_var : use_vars[bb]) {
                        if (tmp_live_in.find(use_var) == tmp_live_in.end()) {
                            tmp_live_in.insert(use_var);
                        }
                    }
                    for (auto var : tmp_live_in) {
                        if (live_in[bb].find(var) == live_in[bb].end()) {
                            live_in[bb].insert(var);
                            repeat = true;
                        }
                    }
                }
            }
            
            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";   
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}