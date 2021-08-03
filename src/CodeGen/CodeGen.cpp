#include<CodeGen.h>
#include<RegAlloc.h>
#include<queue>
#include<algorithm>
#include<sstream>
#include<string>

// namespace CodeGen{

    std::string CodeGen::global(std::string name){
        return IR2asm::space + ".globl " + name + IR2asm::endl;
    }

    bool CodeGen::iszeroinit(Constant * init){
        if(dynamic_cast<ConstantInt *>(init)){
            return (dynamic_cast<ConstantInt *>(init)->get_value() == 0);
        }
        else{
            auto initalizer = dynamic_cast<ConstantArray *>(init);
            int init_size = initalizer->get_size_of_array();
            for(int i = 0; i < init_size; i++){
                Constant* init_iter = initalizer->get_element_value(i);
                if(!iszeroinit(init_iter))return false;
            }
        }
        return true;
    }

    std::string CodeGen::global_def_gen(Module* module){
        std::string code;
        for(auto var: module->get_global_variable()){
            std::string name = var->get_name();
            bool isconst = var->is_const();
            auto initializer = var->get_init();
            bool isinitialized = (dynamic_cast<ConstantZero *>(initializer) == nullptr);
            bool isarray = (dynamic_cast<ConstantArray *>(initializer) != nullptr);
            int size = var->get_type()->get_size();
            code += IR2asm::space;
            code += ".type " + name + ", %object" + IR2asm::endl;
            code += IR2asm::space;
            if(isinitialized){  //initialized global var
                bool iszeroinit_ = iszeroinit(initializer);
                if(isconst){
                    code += ".section .rodata,\"a\", %progbits" + IR2asm::endl;
                }
                else{
                    if(iszeroinit_){
                        code += ".bss" + IR2asm::endl;
                    }
                    else{
                        code += ".data" + IR2asm::endl;
                    }
                }
                code += global(name);
                code += IR2asm::space;
                code += ".p2align " + std::to_string(int_p2align) + IR2asm::endl;
                code += name + ":" + IR2asm::endl;
                code += IR2asm::space;
                if(!isarray){
                    code += ".long ";
                    code += std::to_string(dynamic_cast<ConstantInt *>(initializer)->get_value());
                    code += IR2asm::endl;
                    code += IR2asm::space;
                }
                else{
                    if(iszeroinit_){
                        code += ".zero ";
                        code += std::to_string(size);
                        code += IR2asm::endl;
                        code += IR2asm::space;
                    }
                    else{
                        auto initalizer_ = dynamic_cast<ConstantArray *>(initializer);
                        int init_size = initalizer_->get_size_of_array();
                        for(int i = 0; i < init_size; i++){
                            Constant* init_iter = initalizer_->get_element_value(i);
                            code += ".long ";
                            code += std::to_string(dynamic_cast<ConstantInt *>(init_iter)->get_value());
                            code += IR2asm::endl;
                            code += IR2asm::space;
                        }
                    }
                }
                code += ".size ";
                code += name + ", ";
                code += std::to_string(size);
                code += IR2asm::endl;
            }
            else{   //uninitialized global var
                code += ".comm ";
                code += name;
                code += ", ";
                code += std::to_string(size);
                code += ", " + std::to_string(int_align);  //int align 4
                code += IR2asm::endl;
            }
            code += IR2asm::endl;
        }
        return code;
    }

    int CodeGen::stack_space_allocation(Function* fun){
        int size = 0;
        int arg_size = 0;
        used_reg.second.clear();
        used_reg.first.clear();
        stack_map.clear();
        arg_on_stack.clear();
        reg2val.clear();
        //arg on stack in reversed sequence
        if(fun->get_num_of_args() > 4){
            for(auto arg: fun->get_args()){
                if(arg->get_arg_no() < 4)continue;
                int type_size = arg->get_type()->get_size();
                arg_on_stack.push_back(new IR2asm::Regbase(IR2asm::sp, arg_size));
                arg_size += type_size;
            }
        }
        if(have_func_call){
            for(auto iter: reg_map){
                Value* vreg = iter.first;
                Interval* interval = iter.second;
                if(interval->reg_num >= 0){
                    if(interval->reg_num > 3){
                        used_reg.second.insert(interval->reg_num);
                    }
                    else{
                        used_reg.first.insert(interval->reg_num);
                    }
                    if(reg2val.find(interval->reg_num)!=reg2val.end()){
                        reg2val.find(interval->reg_num)->second.push_back(vreg);
                    }
                    else{
                        reg2val.insert({interval->reg_num, {vreg}});
                    }
                    continue;
                }
                if(dynamic_cast<Argument*>(vreg)){
                    auto arg = dynamic_cast<Argument*>(vreg);
                    if(arg->get_arg_no() > 3)continue;
                }
                int type_size = vreg->get_type()->get_size();
                size += type_size;
                stack_map.insert({vreg, new IR2asm::Regbase(IR2asm::frame_ptr, -size)});
            }
            used_reg.second.insert(IR2asm::frame_ptr);
            // size += reg_size;
            for(auto inst: fun->get_entry_block()->get_instructions()){
                auto alloc = dynamic_cast<AllocaInst*>(inst);
                if(!alloc)continue;
                int type_size = alloc->get_alloca_type()->get_size();
                size += type_size;
                stack_map.insert({dynamic_cast<Value *>(alloc), new IR2asm::Regbase(IR2asm::frame_ptr, -size)});
            }
        }
        else{
            // stack alloc without frame pointer
            for(auto iter: reg_map){
                Value* vreg = iter.first;
                Interval* interval = iter.second;
                if(interval->reg_num >= 0){
                    if(interval->reg_num > 3){
                        used_reg.second.insert(interval->reg_num);
                    }
                    else{
                        used_reg.first.insert(interval->reg_num);
                    }
                    if(reg2val.find(interval->reg_num)!=reg2val.end()){
                        reg2val.find(interval->reg_num)->second.push_back(vreg);
                    }
                    else{
                        reg2val.insert({interval->reg_num, {vreg}});
                    }
                    continue;
                }
                if(dynamic_cast<Argument*>(vreg)){
                    auto arg = dynamic_cast<Argument*>(vreg);
                    if(arg->get_arg_no() > 3)continue;
                }
                int type_size = vreg->get_type()->get_size();
                size += type_size;
                stack_map.insert({vreg, new IR2asm::Regbase(IR2asm::sp, -size)});
            }
            for(auto inst: fun->get_entry_block()->get_instructions()){
                auto alloc = dynamic_cast<AllocaInst*>(inst);
                if(!alloc)continue;
                int type_size = alloc->get_alloca_type()->get_size();
                size += type_size;
                stack_map.insert({dynamic_cast<Value *>(alloc), new IR2asm::Regbase(IR2asm::sp, -size)});
            }
            for(auto map: stack_map){
                int offset = map.second->get_offset();
                map.second->set_offset(size + offset);
            }
        }
        int reg_store_size = reg_size * (used_reg.second.size() + ((have_func_call)? 1 : 0) );
        for(auto item: arg_on_stack){
            int offset = item->get_offset();
            item->set_offset(offset + reg_store_size + size);
        }
        return size;
    }

    std::string CodeGen::callee_reg_store(Function* fun){
        std::string code;
        if(!used_reg.second.size())return "";
        code += IR2asm::space;
        code += "push {";
        for(auto reg: used_reg.second){
            if(reg <= max_func_reg)continue;
            code += IR2asm::reg_name[reg];
            if(reg == *used_reg.second.rbegin())break;
            code += ", ";
        }
        // code += (IR2asm::frame_ptr).get_code();
        // code += ", ";
        if(have_func_call)code += ", lr}";
        else{
            code += "}";
        }
        code += IR2asm::endl;
        return code;
    }

    std::string CodeGen::callee_reg_restore(Function* fun){
        std::string code;
        if(!used_reg.second.size())return "";
        code += IR2asm::space;
        code += "pop {";
        for(auto reg: used_reg.second){
            if(reg <= max_func_reg)continue;
            code += IR2asm::reg_name[reg];
            if(reg == *used_reg.second.rbegin())break;
            code += ", ";
        }
        // code += (IR2asm::frame_ptr).get_code();
        // code += ", ";
        if(have_func_call)code += ", lr}";
        else{
            code += "}";
        }
        code += IR2asm::endl;
        return code;
    }

    std::string CodeGen::callee_stack_operation_in(Function* fun, int stack_size){
        //TODO: immediate overflow
        std::string code;
        code += IR2asm::space;
        if(have_func_call){
            code += "mov ";
            code += IR2asm::Reg(IR2asm::frame_ptr).get_code();
            code += ", sp";
            // code += std::to_string(2 * int_size);
            code += IR2asm::endl;
            code += IR2asm::space;
        }
        code += "sub sp, sp, #";
        code += std::to_string(stack_size);
        code += IR2asm::endl;
        return code;
    }

    std::string CodeGen::callee_stack_operation_out(Function* fun, int stack_size){
        std::string code;
        code += IR2asm::space;
        if(have_func_call){
            code += "mov sp, ";
            code += IR2asm::Reg(IR2asm::frame_ptr).get_code();
            // code += std::to_string(2 * int_size);
            code += IR2asm::endl;
            return code;
        }
        code += "add sp, sp, #";
        // code += IR2asm::frame_ptr.get_code();
        // code += ", #";
        code += std::to_string(stack_size);
        code += IR2asm::endl;
        return code;
    }

    std::string CodeGen::caller_reg_store(Function* fun,CallInst* call){
        std::string code = "";
        to_save_reg.clear();
//        int arg_num = fun->get_num_of_args();
//        if(arg_num > 4)
        int arg_num = 4;
        if(!used_reg.first.empty()){
            for(int i = 0;i < arg_num;i++){
                if(used_reg.first.find(i) != used_reg.first.end()){
                    bool not_to_save = true;
                    for(auto val:reg2val[i]){
                        not_to_save = not_to_save && !reg_map[val]->covers(call);
                    }
                    if(!not_to_save){
                        if(!call->is_void()){
                            if(reg_map[call]->reg_num==i){
                                continue;
                            }
                        }
                        to_save_reg.push_back(i);
                    }
                }
            }
        }
        if(arg_num==0&&!call->is_void()){
            if(used_reg.first.find(0) != used_reg.first.end()){
                bool not_to_save = true;
                for(auto val:reg2val[0]){
                    not_to_save = not_to_save && !reg_map[val]->covers(call);
                }
                if(!not_to_save){
                    if(reg_map[call]->reg_num!=0){
                        to_save_reg.push_back(0);
                    }
                }
            }
        }
        //TODO:debug caller save reg
        if(used_reg.second.find(12) != used_reg.second.end()){
            bool not_to_save = true;
            for(auto val:reg2val[12]){
                not_to_save = not_to_save && !reg_map[val]->covers(call);
            }
            if(!not_to_save){
                if(call->is_void() || reg_map[call]->reg_num!=12){
                    to_save_reg.push_back(12);
                }
            }
        }
        if(!to_save_reg.empty()){
//            code += IR2asm::space;
//            code += "push {";
//            int save_size = to_save_reg.size();
//            int i = 0;
//            for(; i < save_size - 1; i++){
//                code += IR2asm::Reg(to_save_reg[i]).get_code();
//                code += ", ";
//            }
//            code += IR2asm::Reg(to_save_reg[save_size-1]).get_code();
//            code += "}";
//            code += IR2asm::endl;
            code += push_regs(to_save_reg);
        }
        return code;
    }

    std::string CodeGen::caller_reg_restore(Function* fun, CallInst* call){
        std::string code = "";
        int arg_num = fun->get_num_of_args();

        if(!call->is_void()){
            int ret_id = reg_map[call]->reg_num;
            if(ret_id!=0){
                if(ret_id > 0){
                    code += IR2asm::space;
                    code += "mov " + IR2asm::Reg(ret_id).get_code();
                    code += ", ";
                    code += IR2asm::Reg(0).get_code();
                    code += IR2asm::endl;
                }else{
                    code += IR2asm::space;
                    code += "str ";
                    code += IR2asm::Reg(0).get_code();
                    code += ", ";
                    code += stack_map[call]->get_ofst_code(sp_extra_ofst);
                    code += IR2asm::endl;
                }
            }
        }        

        if(arg_num > 4)arg_num = 4;
        if(!to_save_reg.empty()){
//            code += IR2asm::space;
//            code += "pop {";
//            auto save_size = to_save_reg.size();
//            for(int i = 0; i < save_size - 1; i++){
//                code += IR2asm::Reg(to_save_reg[i]).get_code();
//                code += ", ";
//            }
//            code += IR2asm::Reg(to_save_reg[save_size-1]).get_code();
//            code += "}";
//            code += IR2asm::endl;
            code += pop_regs(to_save_reg);
        }
        return code;
    }

    void CodeGen::make_global_table(Module* module){
        for(auto var: module->get_global_variable()){
            for(auto use: var->get_use_list()){
                Function* func_;
                func_ = dynamic_cast<Instruction *>(use.val_)->get_parent()->get_parent();
                // std::cout << func_->get_name() << ":" << var->get_name() << "\n";
                if(global_variable_use.find(func_) != global_variable_use.end()){
                    global_variable_use.find(func_)->second.insert(var);
                }
                else{
                    global_variable_use.insert({func_, {var}});
                }
            }
        }
    }
    
    std::string CodeGen::print_global_table(){
        std::string code;
        for(auto iter: global_variable_table){
            GlobalVariable* var = iter.first;
            IR2asm::label label = *iter.second;
            code += label.get_code();
            code += ":" + IR2asm::endl;
            code += IR2asm::space;
            code += ".long ";
            code += var->get_name();
            code += IR2asm::endl;
        }
        return code;
    }

    std::string CodeGen::module_gen(Module* module){
        std::string code;
        std::string globaldef;
        globaldef += global_def_gen(module);
        // std::cout << code;
        RegAllocDriver driver = RegAllocDriver(module);
        driver.compute_reg_alloc();
        make_global_table(module);
        func_no = 0;
        code += IR2asm::space + ".text " + IR2asm::endl;
        for(auto func_: module->get_functions()){
            if(func_->get_basic_blocks().empty())continue;
            reg_map = driver.get_reg_alloc_in_func(func_);
            code += function_gen(func_) + IR2asm::endl;
            func_no++;
        }
        //TODO: *other machine infomation
        return code + globaldef;
    }

    void CodeGen::make_linear_bb(Function* fun){
        //sort bb and make bb label, put in CodeGen::bb_label
        //label gen, name mangling as bbx_y for yth bb in function no.x .
        bb_label.clear();
        linear_bb.clear();
        bb_no = -1;
        BasicBlock* ret_bb;
        IR2asm::label *newlabel;
        std::string label_str;
        for(auto bb: fun->get_basic_blocks()){
            if(bb != fun->get_entry_block() && !bb->get_terminator()->is_ret()){
                label_str = "bb" + std::to_string(func_no) + "_" + std::to_string(bb_no);
                newlabel = new IR2asm::label(label_str);
                bb_label.insert({bb, newlabel});
            }
            else if(bb == fun->get_entry_block() && bb->get_terminator()->is_ret()){
                bb_label.insert({bb, new IR2asm::label("")});
                linear_bb.push_back(bb);
                return;
            }
            else if(bb == fun->get_entry_block()){
                // bb_label.insert({bb, new IR2asm::label(fun->get_name())});
                bb_label.insert({bb, new IR2asm::label("")});
            }
            else{
                ret_bb = bb;
                continue;
            }
            linear_bb.push_back(bb);
            bb_no++;
        }
        label_str = "bb" + std::to_string(func_no) + "_" + std::to_string(bb_no);
        newlabel = new IR2asm::label(label_str);
        bb_label.insert({ret_bb, newlabel});
        linear_bb.push_back(ret_bb);
        return;
    }

    void CodeGen::global_label_gen(Function* fun){
        //global varibal address store after program(.LCPIx_y), fill in CodeGen::global_variable_table
        if(global_variable_use.find(fun) == global_variable_use.end()){
            global_variable_table.clear();
            return;        
        }
        auto used_global = global_variable_use.find(fun)->second;
        global_variable_table.clear();
        label_no = 0;
        for(auto var: used_global){
            std::string label_str = "Addr" + std::to_string(func_no) + "_" + std::to_string(label_no);
            IR2asm::label* new_label = new IR2asm::label(label_str);
            label_no++;
            global_variable_table.insert({var, new_label});
        }
    }

    void CodeGen::func_call_check(Function* fun){
        max_arg_size = 0;
        have_func_call = false;
        for(auto bb: fun->get_basic_blocks()){
            for(auto inst: bb->get_instructions()){
                auto call = dynamic_cast<CallInst*>(inst);
                if(!call)continue;
                int arg_size = 0;
                auto callee = dynamic_cast<Function *>(call->get_operand(0));
                for(auto arg: callee->get_args()){
                    arg_size += arg->get_type()->get_size();
                }
                if(arg_size > max_arg_size)max_arg_size = arg_size;
                have_func_call = true;
            }
        }
        return;
    }

    std::string CodeGen::arg_move(CallInst* call){
        //arg on stack in reversed sequence
        std::string regcode;
        std::string memcode;
        std::queue<Value *> push_queue;//for sequence changing
        auto fun = dynamic_cast<Function *>(call->get_operand(0));
        int i = 0;
        for(auto arg: call->get_operands()){
            if(dynamic_cast<Function *>(arg))continue;
            if(i < 4){
                regcode += IR2asm::space;
                if(dynamic_cast<ConstantInt *>(arg)){
                    regcode += "ldr ";
                    regcode += IR2asm::Reg(i).get_code();
                    regcode += ", =";
                    regcode += std::to_string(dynamic_cast<ConstantInt *>(arg)->get_value());
                    regcode += IR2asm::endl;
                    i++;
                    continue;
                }
                auto reg = (reg_map).find(arg)->second->reg_num;
                IR2asm::Reg* preg;
                if(reg >= 0){
                    if(reg == i){
                        i++;
                        continue;
                    }
                    else{
                        preg = new IR2asm::Reg(reg);
                    }
                    regcode += "mov ";
                    regcode += IR2asm::Reg(i).get_code();
                    regcode += ", ";
                    regcode += preg->get_code();
                    regcode += IR2asm::endl;
                }
                else{
                    regcode += "ldr ";
                    regcode += IR2asm::Reg(i).get_code();
                    regcode += ", ";
                    regcode += stack_map.find(arg)->second->get_ofst_code(sp_extra_ofst);
                    regcode += IR2asm::endl;
                }
            }
            else{
                push_queue.push(arg);
            }
            i++;
        }
        int offset = 0;
        while(!push_queue.empty()){
            Value* arg = push_queue.front();
            push_queue.pop();
            if(dynamic_cast<ConstantInt *>(arg)){
                memcode += IR2asm::space;
                memcode += "ldr r0, =";
                memcode += std::to_string(dynamic_cast<ConstantInt *>(arg)->get_value());
                memcode += IR2asm::endl;
                memcode += IR2asm::space;
                memcode += "str r0, ";
                memcode += IR2asm::Regbase(IR2asm::Reg(IR2asm::sp), offset).get_ofst_code(sp_extra_ofst);
                offset += arg->get_type()->get_size();
                memcode += IR2asm::endl;
                continue;
            }
            auto reg = (reg_map).find(arg)->second->reg_num;
            if(reg >= 0){
                //TODO: check push?
                memcode += IR2asm::space;
                memcode += "str ";
                memcode += IR2asm::Reg(reg).get_code();
                memcode += ", ";
                memcode += IR2asm::Regbase(IR2asm::Reg(IR2asm::sp), offset).get_ofst_code(sp_extra_ofst);
                offset += arg->get_type()->get_size();
                memcode += IR2asm::endl;
            }
            else{
                auto srcaddr = stack_map.find(arg)->second;
                memcode += IR2asm::space;
                memcode += "ldr r0, ";
                memcode += srcaddr->get_ofst_code(sp_extra_ofst);
                memcode += IR2asm::endl;
                memcode += IR2asm::space;
                memcode += "str r0, ";
                memcode += IR2asm::Regbase(IR2asm::Reg(IR2asm::sp), offset).get_ofst_code(sp_extra_ofst);
                offset += arg->get_type()->get_size();
                memcode += IR2asm::endl;
            }
        }
        return memcode + regcode;
    }

    std::string CodeGen::callee_arg_move(Function* fun){
        std::string code;
        for(auto arg: fun->get_args()){
            int reg;
            if(reg_map.find(arg)!= reg_map.end()){
                reg = reg_map[arg]->reg_num;
            }
            else{
//                reg = -1;
                continue;
            }
            if(arg->get_arg_no() < 4){
                if(reg >= 0){
                    code += IR2asm::space;
                    code += "mov ";
                    code += IR2asm::Reg(reg).get_code();
                    code += ", ";
                    code += IR2asm::Reg(arg->get_arg_no()).get_code();
                    code += IR2asm::endl;
                }
                else{
                    code += IR2asm::space;
                    code += "str ";
                    code += IR2asm::Reg(arg->get_arg_no()).get_code();
                    code += ", ";
                    code += stack_map[arg]->get_ofst_code(sp_extra_ofst);
                    code += IR2asm::endl;
                }
            }
            else{
                if(reg < 0)continue;
                code += IR2asm::space;
                code += "ldr ";
                code += IR2asm::Reg(reg).get_code();
                code += ", ";
                code += arg_on_stack[arg->get_arg_no() - 4]->get_code();
                code += IR2asm::endl;
            }
        }
        return code;
    }

    std::string CodeGen::function_gen(Function* fun){
        std::string code;
        sp_extra_ofst = 0;
        global_label_gen(fun);
        make_linear_bb(fun);
        func_call_check(fun);
        int stack_size = stack_space_allocation(fun) 
                + std::max(max_arg_size - 4 * reg_size, 0);
        code += IR2asm::space + ".globl " + fun->get_name() + IR2asm::endl;
        code += IR2asm::space + ".p2align " + std::to_string(int_p2align) + IR2asm::endl;
        code += IR2asm::space + ".type " + fun->get_name() + ", %function" + IR2asm::endl;
        code += fun->get_name() + ":" + IR2asm::endl;
        code += callee_reg_store(fun);
        if(stack_size)code += callee_stack_operation_in(fun, stack_size);
        code += callee_arg_move(fun);

        //TODO: basicblock gen
        for(auto bb: linear_bb){
            code += bb_gen(bb);
        }
        if(stack_size)code += callee_stack_operation_out(fun, stack_size);
        code += callee_reg_restore(fun);
        code += IR2asm::space + "bx lr" + IR2asm::endl;
        code += print_global_table();
        // std::cout << code << IR2asm::endl;
        return code;
    }

    std::string CodeGen::push_regs(std::vector<int> &reg_list) {
        std::string code;
        code += IR2asm::space;
        code += "push {";
        for(auto reg: reg_list){
            code += IR2asm::Reg(reg).get_code();
            if(reg != *reg_list.rbegin())code += ", ";
        }
        code += "}" + IR2asm::endl;
        sp_extra_ofst += reg_list.size() * reg_size;
        return code;
    }

    std::string CodeGen::pop_regs(std::vector<int> &reg_list) {
        std::string code;
        code += IR2asm::space;
        code += "pop {";
        for(auto reg: reg_list){
            code += IR2asm::Reg(reg).get_code();
            if(reg != *reg_list.rbegin())code += ", ";
        }
        code += "}" + IR2asm::endl;
        sp_extra_ofst -= reg_list.size() * reg_size;
        return code;
    }

    std::string CodeGen::bb_gen(BasicBlock* bb){
        std::string code;
        if(bb_label[bb]->get_code() != ""){
            code += bb_label[bb]->get_code()+":"+IR2asm::endl;
        }
        Instruction* br_inst = nullptr;
        for(auto inst : bb->get_instructions()){
            if(inst->isTerminator()){
                br_inst = inst;
                break;
            }
            if(dynamic_cast<CallInst*>(inst)){
                auto call_inst = dynamic_cast<CallInst*>(inst);        
                code += caller_reg_store(bb->get_parent(),call_inst);
                code += arg_move(call_inst);
                code += instr_gen(call_inst);
                code += caller_reg_restore(bb->get_parent(),call_inst);
            }else if(instr_may_need_push_stack(inst)){
                std::vector<int> store_list = {};
                std::set<Value*> to_store_set = {};
                std::set<Value*> to_ld_set = {};
                std::set<Interval*> interval_set = {};
                bool use_target = false;
                if(!inst->is_void() && !dynamic_cast<AllocaInst *>(inst)){
                    auto reg_inter = reg_map[inst];
                    if(reg_inter->reg_num<0){
                        reg_inter->reg_num = store_list.size();
                        auto it = std::find(store_list.begin(),store_list.end(),reg_inter->reg_num);
                        if(it==store_list.end()){
                            store_list.push_back(reg_inter->reg_num);
                        }
                        interval_set.insert(reg_inter);
                        to_store_set.insert(inst);
                        use_target = true;
                    }
                }
                for(auto opr:inst->get_operands()){
                    if(dynamic_cast<Constant*>(opr) || 
                    dynamic_cast<BasicBlock *>(opr) ||
                    dynamic_cast<GlobalVariable *>(opr) ||
                    dynamic_cast<AllocaInst *>(opr)){
                        continue;
                    }
                    auto reg_inter = reg_map[opr];
                    if(reg_inter->reg_num<0){
                        if(use_target){
                            reg_inter->reg_num = store_list.size() - 1;
                        }else{
                            reg_inter->reg_num = store_list.size();
                        }
                        auto it = std::find(store_list.begin(),store_list.end(),reg_inter->reg_num);
                        if(it==store_list.end()){
                            store_list.push_back(reg_inter->reg_num);
                        }
                        interval_set.insert(reg_inter);
                        to_ld_set.insert(opr);
                    }
                }
                if(!store_list.empty()){
//                    code += IR2asm::space;
//                    code += "push {";
//                    int lst_size = store_list.size() - 1;
//                    for(int i = 0;i < lst_size;i++){
//                        code += IR2asm::Reg(store_list[i]).get_code();
//                        code += ", ";
//                    }
//                    code += IR2asm::Reg(store_list[lst_size]).get_code();
//                    code += "}";
//                    code += IR2asm::endl;
                    code += push_regs(store_list);
                }
                for(auto opr:to_ld_set){
                    code += IR2asm::space;
                    code += "ldr ";
                    code += IR2asm::Reg(reg_map[opr]->reg_num).get_code() +", "+
                            stack_map[opr]->get_ofst_code(sp_extra_ofst);
                    code += IR2asm::endl;
                }

                code += instr_gen(inst);

                for(auto opr:to_store_set){
                    code += IR2asm::space;
                    code += "str ";
                    code += IR2asm::Reg(reg_map[opr]->reg_num).get_code() +", "+
                            stack_map[opr]->get_ofst_code(sp_extra_ofst);
                    code += IR2asm::endl;
                }

                if(!store_list.empty()){
//                    code += IR2asm::space;
//                    code += "pop {";
//                    int lst_size = store_list.size() - 1;
//                    for(int i = 0;i < lst_size;i++){
//                        code += IR2asm::Reg(store_list[i]).get_code();
//                        code += ", ";
//                    }
//                    code += IR2asm::Reg(store_list[lst_size]).get_code();
//                    code += "}";
//                    code += IR2asm::endl;
                    code += pop_regs(store_list);
                }

                for(auto inter:interval_set){
                    inter->reg_num = -1;
                }
            }else{
                code += instr_gen(inst);
            }
        }

        code += phi_union(bb, br_inst);
        // code += instr_gen(br_inst);
        return code;
        //TODO:PHI INST CHECK
    }

    void spilt_str(const std::string& s, std::vector<std::string>& sv, const char delim = ' ') {
        sv.clear();
        std::istringstream iss(s);
        std::string temp;
        while (std::getline(iss, temp, delim)) {
            sv.emplace_back(std::move(temp));
        }
        return;
    }

    std::string CodeGen::phi_union(BasicBlock* bb, Instruction* br_inst){
        //TODO:right?
        if(dynamic_cast<ReturnInst *>(br_inst)){
            return instr_gen(br_inst);
        }
        std::string cmp;
        std::string succ_code;
        std::string fail_code;
        std::string succ_br;
        std::string fail_br;
        std::string* code = &succ_code;
        bool is_succ = true;
        bool is_cmpbr = false;
        CmpBrInst* cmpbr = dynamic_cast<CmpBrInst*>(br_inst);
        BasicBlock* succ_bb;
        BasicBlock* fail_bb;

        std::map<Value*,std::set<Value*>> opr2phi;
        //TODO:PHI INST CHECK
        std::set<Value*> sux_bb_phi = {};
        std::vector<std::string> cmpbr_inst;
        std::string cmpbr_code = instr_gen(br_inst);
        spilt_str(cmpbr_code, cmpbr_inst, IR2asm::endl[0]);

        if(cmpbr){
            is_cmpbr = true;
            succ_bb = dynamic_cast<BasicBlock*>(cmpbr->get_operand(2));
            fail_bb = dynamic_cast<BasicBlock*>(cmpbr->get_operand(3));
            cmp += cmpbr_inst[0] + IR2asm::endl;
            succ_br += cmpbr_inst[1] + IR2asm::endl;
            fail_br += cmpbr_inst[2] + IR2asm::endl;
        }
        else{
            succ_bb = dynamic_cast<BasicBlock*>(br_inst->get_operand(0));
            succ_br += cmpbr_inst[0] + IR2asm::endl;
        }

        for(auto sux:bb->get_succ_basic_blocks()){
            if(sux == succ_bb){
                code = &succ_code;
            }
            else{
                code = &fail_code;
            }
            sux_bb_phi.clear();
            opr2phi.clear();
            for(auto inst:sux->get_instructions()){
                if(inst->is_phi()){
                    Value* lst_val = nullptr;
                    for(auto opr:inst->get_operands()){
                        if(dynamic_cast<BasicBlock*>(opr)){
                            auto this_bb = dynamic_cast<BasicBlock*>(opr);
                            if(this_bb==bb){
                                sux_bb_phi.insert(lst_val);
                                if(opr2phi.find(lst_val)==opr2phi.end()){
                                    opr2phi[lst_val] = std::set<Value*>();
                                    opr2phi[lst_val].insert(inst);
                                }
                                else{
                                    opr2phi[lst_val].insert(inst);
                                }
                            }
                        }else{
                            lst_val = opr;
                        }
                    }
                }else{
                    break;
                }
            }
            for(auto opr:sux_bb_phi){
                if(dynamic_cast<ConstantInt*>(opr)){
                    auto const_opr = dynamic_cast<ConstantInt*>(opr);
                    int const_val = const_opr->get_value();
                    for(auto target:opr2phi[opr]){
                        auto tar_inter = reg_map[target];
                        if(tar_inter->reg_num>=0){
                            *code += IR2asm::space;
                            *code += "LDR ";
                            *code += IR2asm::Reg(tar_inter->reg_num).get_code();
                            *code += ",=";
                            *code += std::to_string(const_val);
                            *code += IR2asm::endl;
                        }else{
    //                        code += IR2asm::space+"push {r0}"+IR2asm::endl;
                            std::vector<int> save_reg = {0};
                            *code += push_regs(save_reg);
                            *code += IR2asm::space;
                            *code += "LDR r0,=";
                            *code += std::to_string(const_val);
                            *code += IR2asm::endl;
                            *code += IR2asm::space;
                            *code += "str r0";
                            *code += ", ";
                            *code += stack_map[target]->get_ofst_code(sp_extra_ofst);
                            *code += IR2asm::endl;
    //                        code += IR2asm::space+"pop {r0}"+IR2asm::endl;
                            *code += pop_regs(save_reg);
                        }
                    }
                }else{
                    if(reg_map[opr]->reg_num>=0){
                        for(auto target:opr2phi[opr]){
                            auto tar_inter = reg_map[target];
                            if(tar_inter->reg_num>=0){
                                if(tar_inter->reg_num!=reg_map[opr]->reg_num){
                                    *code += IR2asm::space;
                                    *code += "mov ";
                                    *code += IR2asm::Reg(tar_inter->reg_num).get_code();
                                    *code += ", ";
                                    *code += IR2asm::Reg(reg_map[opr]->reg_num).get_code();
                                    *code += IR2asm::endl;
                                }
                            }else{
                                *code += IR2asm::space;
                                *code += "str ";
                                *code += IR2asm::Reg(reg_map[opr]->reg_num).get_code();
                                *code += ", ";
                                *code += stack_map[target]->get_ofst_code(sp_extra_ofst);
                                *code += IR2asm::endl;
                            }
                        }
                    }else{
                        for(auto target:opr2phi[opr]){
                            auto tar_inter = reg_map[target];
                            if(tar_inter->reg_num>=0){
                                *code += IR2asm::space;
                                *code += "ldr ";
                                *code += IR2asm::Reg(tar_inter->reg_num).get_code();
                                *code += ", ";
                                *code += stack_map[opr]->get_ofst_code(sp_extra_ofst);
                                *code += IR2asm::endl;
                            }else{
    //                            code += IR2asm::space;
    //                            code += "push {lr}";
    //                            code += IR2asm::endl;
                                std::vector<int> save_reg = {0};
                                *code += push_regs(save_reg);
                                *code += IR2asm::space;
                                *code += "ldr r0, ";
                                *code += stack_map[opr]->get_ofst_code(sp_extra_ofst);
                                *code += IR2asm::endl;
                                *code += IR2asm::space;
                                *code += "str lr, ";
                                *code += stack_map[target]->get_ofst_code(sp_extra_ofst);
                                *code += IR2asm::endl;
    //                            code += IR2asm::space;
    //                            code += "pop {lr}";
    //                            code += IR2asm::endl;
                                *code += pop_regs(save_reg);
                            }
                        }
                    }
                }
            }
        }
        return cmp + succ_code + succ_br + fail_code + fail_br;
    }

    //TODO: return bb

    std::string CodeGen::instr_gen(Instruction * inst){
        std::string code;
        //TODO: call functions in IR2asm , deal with phi inst(mov inst)
        // may have many bugs
        auto instr_type = inst->get_instr_type();
        switch (instr_type)
        {
            case Instruction::ret:
                if (inst->get_operands().empty()) {
                    //code += IR2asm::ret();
                } else {
                    auto ret_val = inst->get_operand(0);
                    auto const_ret_val = dynamic_cast<ConstantInt*>(ret_val);
                    if (!const_ret_val&&get_asm_reg(ret_val)->get_id() == 0) {
//                        code += IR2asm::ret();
                    } else {
                        if (const_ret_val) {
                            code += IR2asm::ret(get_asm_const(const_ret_val));
                        } else {
                            if (get_asm_reg(ret_val)->get_id() < 0) {
                                code += IR2asm::ret(stack_map[ret_val]);
                            } else {
                                code += IR2asm::ret(get_asm_reg(ret_val));
                            }
                        }
                    }
                }
                break;
            case Instruction::br:
                if (inst->get_num_operand() == 1) {
                    code += IR2asm::b(bb_label[dynamic_cast<BasicBlock*>(inst->get_operand(0))]);
                }
                break;
            case Instruction::add: {
                    auto op1 = inst->get_operand(0);
                    auto op2 = inst->get_operand(1);
                    auto const_op1 = dynamic_cast<ConstantInt*>(op1);
                    auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                    Value *operand1;
                    IR2asm::Operand2 *operand2;
                    if (const_op1) {
                        operand1 = op2;
                        operand2 = new IR2asm::Operand2(const_op1->get_value());
                    } else {
                        operand1 = op1;
                        if (const_op2) {
                            operand2 = new IR2asm::Operand2(const_op2->get_value());
                        } else {
                            operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                        }
                    }
                    code += IR2asm::add(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                }
                break;
                case Instruction::sub: {
                    auto op1 = inst->get_operand(0);
                    auto op2 = inst->get_operand(1);
                    auto const_op1 = dynamic_cast<ConstantInt*>(op1);
                    auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                    Value *operand1;
                    IR2asm::Operand2 *operand2;
                    if (const_op1) {
                        operand1 = op2;
                        operand2 = new IR2asm::Operand2(const_op1->get_value());
                        code += IR2asm::r_sub(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    } else {
                        operand1 = op1;
                        if (const_op2) {
                            operand2 = new IR2asm::Operand2(const_op2->get_value());
                        } else {
                            operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                        }
                        code += IR2asm::sub(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                }
                break;
                case Instruction::mul: {
                    auto op1 = inst->get_operand(0);
                    auto op2 = inst->get_operand(1);
                    auto const_op1 = dynamic_cast<ConstantInt*>(op1);
                    auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                    Value *operand1;
                    IR2asm::Operand2 *operand2;
                    if (const_op1) {
                        operand1 = op2;
                        operand2 = new IR2asm::Operand2(const_op1->get_value());
                    } else {
                        operand1 = op1;
                        if (const_op2) {
                            operand2 = new IR2asm::Operand2(const_op2->get_value());
                        } else {
                            operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                        }
                    }
                    code += IR2asm::mul(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                }
                break;
                case Instruction::sdiv: { // divide consant can be optimized
                    auto op1 = inst->get_operand(0);
                    auto op2 = inst->get_operand(1);
                    auto operand1 = op1;
                    IR2asm::Operand2* operand2;
                    auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                    if (const_op2) {
                        operand2 = new IR2asm::Operand2(get_asm_const(const_op2)->get_val());
                    } else {
                        operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                    }
                    code += IR2asm::sdiv(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                }
                break;
                case Instruction::srem: // srem -> sdiv and msub
                break;
                case Instruction::alloca:   // has done before
                break;
                case Instruction::load: {
                    auto global_addr = dynamic_cast<GlobalVariable*>(inst->get_operand(0));
                    if (global_addr) {
                        code += IR2asm::load(get_asm_reg(inst), global_variable_table[global_addr]);
                        code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), 0));
                    } else {
                        code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst->get_operand(0)), 0));
                    }
                }
                break;
                case Instruction::store: {
                    auto global_addr = dynamic_cast<GlobalVariable*>(inst->get_operand(1));
                    int ld_reg_id = 0;
                    bool need_pop = true;
                    std::vector<int> tmp_reg;
                    if (global_addr) {
                        auto unused_reg = inst->get_parent()->get_parent()->get_unused_reg_num();
                        if(!unused_reg.empty()){
                            int reg_id = *unused_reg.begin();
                            ld_reg_id = reg_id;
                            if(reg_id<=3&&reg_id>=0){
                                need_pop = false;
                            }
                        }
                        if(need_pop){
//                            code += IR2asm::space;
//                            code += "push {";
//                            code += IR2asm::Reg(ld_reg_id).get_code();
//                            code += "}";
//                            code += IR2asm::endl;
                            tmp_reg.push_back(ld_reg_id);
                            code += push_regs(tmp_reg);
                        }
                        code += IR2asm::load(new IR2asm::Reg(ld_reg_id),global_variable_table[global_addr]);
                    }else{
                        need_pop = false;
                        ld_reg_id = get_asm_reg(inst->get_operand(1))->get_id();
                    }
                    code += IR2asm::store(get_asm_reg(inst->get_operand((0))), new IR2asm::Regbase(IR2asm::Reg(ld_reg_id), 0));
                    if(need_pop){
//                        code += IR2asm::space;
//                        code += "pop {";
//                        code += IR2asm::Reg(ld_reg_id).get_code();
//                        code += "}";
//                        code += IR2asm::endl;
                        code += pop_regs(tmp_reg);
                    }
                }
                break;
                case Instruction::cmp: // DONE in cmpbr
                break;
                case Instruction::phi:  // has done before
                break;
                case Instruction::call:
                    code += IR2asm::call(new IR2asm::label(inst->get_operand(0)->get_name()));
                    break;
                    case Instruction::getelementptr: {
                        IR2asm::Location *addr;
                        auto arg_addr = dynamic_cast<Argument*>(inst->get_operand(0));
                        if (arg_addr) {
                            auto arg_num = arg_addr->get_arg_no();
                            if (arg_num < 4) {
                                code += IR2asm::mov(get_asm_reg(inst), new IR2asm::Operand2(*get_asm_reg(inst->get_operand(0))));
                            } else {
                                code += IR2asm::getelementptr(get_asm_reg(inst), arg_on_stack[arg_num - 4]);
                            }
                        } else {
                            auto global_addr = dynamic_cast<GlobalVariable*>(inst->get_operand(0));
                            if (global_addr) {
                                addr = global_variable_table[global_addr];
                            } else {
                                if (dynamic_cast<AllocaInst*>(inst->get_operand(0))) {
                                    addr = stack_map[inst->get_operand(0)];
                                } else {
                                    addr = new IR2asm::Regbase(*get_asm_reg(inst), 0);
                                }
                            }
                            code += IR2asm::getelementptr(get_asm_reg(inst), addr);
                        }
                    }
                    break;
                    case Instruction::land: {
                        auto op1 = inst->get_operand(0);
                        auto op2 = inst->get_operand(1);
                        auto const_op1 = dynamic_cast<ConstantInt*>(op1);
                        auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                        Value *operand1;
                        IR2asm::Operand2 *operand2;
                        if (const_op1) {
                            operand1 = op2;
                            operand2 = new IR2asm::Operand2(const_op1->get_value());
                        } else {
                            operand1 = op1;
                            if (const_op2) {
                                operand2 = new IR2asm::Operand2(const_op2->get_value());
                            } else {
                                operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                            }
                        }
                        code += IR2asm::land(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                    break;
                    case Instruction::lor: {
                        auto op1 = inst->get_operand(0);
                        auto op2 = inst->get_operand(1);
                        auto const_op1 = dynamic_cast<ConstantInt*>(op1);
                        auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                        Value *operand1;
                        IR2asm::Operand2 *operand2;
                        if (const_op1) {
                            operand1 = op2;
                            operand2 = new IR2asm::Operand2(const_op1->get_value());
                        } else {
                            operand1 = op1;
                            if (const_op2) {
                                operand2 = new IR2asm::Operand2(const_op2->get_value());
                            } else {
                                operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                            }
                        }
                        code += IR2asm::lor(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                    break;
                    case Instruction::lxor: {
                        auto op1 = inst->get_operand(0);
                        auto op2 = inst->get_operand(1);
                        auto const_op1 = dynamic_cast<ConstantInt*>(op1);
                        auto const_op2 = dynamic_cast<ConstantInt*>(op2);
                        Value *operand1;
                        IR2asm::Operand2 *operand2;
                        if (const_op1) {
                            operand1 = op2;
                            operand2 = new IR2asm::Operand2(const_op1->get_value());
                        } else {
                            operand1 = op1;
                            if (const_op2) {
                                operand2 = new IR2asm::Operand2(const_op2->get_value());
                            } else {
                                operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                            }
                        }
                        code += IR2asm::lxor(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                    break;
                    case Instruction::zext:
                        break;
                    case Instruction::asr: {
                        auto op1 = inst->get_operand(0);
                        auto op2 = inst->get_operand(1);
                        auto operand1 = op1;
                        auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                        code += IR2asm::asr(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                    break;
                    case Instruction::lsl: {
                        auto op1 = inst->get_operand(0);
                        auto op2 = inst->get_operand(1);
                        auto operand1 = op1;
                        auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                        code += IR2asm::lsl(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                    break;
                    case Instruction::lsr: {
                        auto op1 = inst->get_operand(0);
                        auto op2 = inst->get_operand(1);
                        auto operand1 = op1;
                        auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
                        code += IR2asm::lsr(get_asm_reg(inst), get_asm_reg(operand1), operand2);
                    }
                    break;
                    case Instruction::cmpbr: {
                        auto cmpbr_inst = dynamic_cast<CmpBrInst*>(inst);
                        auto cond1 = inst->get_operand(0);
                        auto cond2 = inst->get_operand(1);
                        auto cmp_op = cmpbr_inst->get_cmp_op();
                        auto true_bb = dynamic_cast<BasicBlock*>(inst->get_operand(2));
                        auto false_bb = dynamic_cast<BasicBlock*>(inst->get_operand(3));
                        auto const_cond1 = dynamic_cast<ConstantInt*>(cond1);
                        auto const_cond2 = dynamic_cast<ConstantInt*>(cond2);
                        IR2asm::Reg *operand1;
                        IR2asm::Operand2 *operand2;
                        switch (cmp_op)
                        {
                            case CmpBrInst::CmpOp::EQ: {
                                if (const_cond1) {
                                    operand1 = get_asm_reg(cond2);
                                    operand2 = new IR2asm::Operand2(const_cond1->get_value());
                                } else {
                                    operand1 = get_asm_reg(cond1);
                                    if (const_cond2) {
                                        operand2 = new IR2asm::Operand2(const_cond2->get_value());
                                    } else {
                                        operand2 = new IR2asm::Operand2(*get_asm_reg(cond2));
                                    }
                                }
                                code += IR2asm::cmp(operand1, operand2);
                                code += IR2asm::beq(bb_label[true_bb]);
                                code += IR2asm::b(bb_label[false_bb]);
                            }
                            break;
                            case CmpBrInst::CmpOp::NE: {
                                if (const_cond1) {
                                    operand1 = get_asm_reg(cond2);
                                    operand2 = new IR2asm::Operand2(const_cond1->get_value());
                                } else {
                                    operand1 = get_asm_reg(cond1);
                                    if (const_cond2) {
                                        operand2 = new IR2asm::Operand2(const_cond2->get_value());
                                    } else {
                                        operand2 = new IR2asm::Operand2(*get_asm_reg(cond2));
                                    }
                                }
                                code += IR2asm::cmp(operand1, operand2);
                                code += IR2asm::bne(bb_label[true_bb]);
                                code += IR2asm::b(bb_label[false_bb]);
                            }
                            break;
                            case CmpBrInst::CmpOp::GT: {
                                if (const_cond1) {
                                    operand1 = get_asm_reg(cond2);
                                    operand2 = new IR2asm::Operand2(const_cond1->get_value());
                                } else {
                                    operand1 = get_asm_reg(cond1);
                                    if (const_cond2) {
                                        operand2 = new IR2asm::Operand2(const_cond2->get_value());
                                    } else {
                                        operand2 = new IR2asm::Operand2(*get_asm_reg(cond2));
                                    }
                                }
                                code += IR2asm::cmp(operand1, operand2);
                                if (const_cond1) {
                                    code += IR2asm::ble(bb_label[true_bb]);
                                } else {
                                    code += IR2asm::bgt(bb_label[true_bb]);
                                }
                                code += IR2asm::b(bb_label[false_bb]);
                            }
                            break;
                            case CmpBrInst::CmpOp::GE: {
                                if (const_cond1) {
                                    operand1 = get_asm_reg(cond2);
                                    operand2 = new IR2asm::Operand2(const_cond1->get_value());
                                } else {
                                    operand1 = get_asm_reg(cond1);
                                    if (const_cond2) {
                                        operand2 = new IR2asm::Operand2(const_cond2->get_value());
                                    } else {
                                        operand2 = new IR2asm::Operand2(*get_asm_reg(cond2));
                                    }
                                }
                                code += IR2asm::cmp(operand1, operand2);
                                if (const_cond1) {
                                    code += IR2asm::blt(bb_label[true_bb]);
                                } else {
                                    code += IR2asm::bge(bb_label[true_bb]);
                                }
                                code += IR2asm::b(bb_label[false_bb]);
                            }
                            break;
                            case CmpBrInst::CmpOp::LT: {
                                if (const_cond1) {
                                    operand1 = get_asm_reg(cond2);
                                    operand2 = new IR2asm::Operand2(const_cond1->get_value());
                                } else {
                                    operand1 = get_asm_reg(cond1);
                                    if (const_cond2) {
                                        operand2 = new IR2asm::Operand2(const_cond2->get_value());
                                    } else {
                                        operand2 = new IR2asm::Operand2(*get_asm_reg(cond2));
                                    }
                                }
                                code += IR2asm::cmp(operand1, operand2);
                                if (const_cond1) {
                                    code += IR2asm::bge(bb_label[true_bb]);
                                } else {
                                    code += IR2asm::blt(bb_label[true_bb]);
                                }
                                code += IR2asm::b(bb_label[false_bb]);
                            }
                            break;
                            case CmpBrInst::CmpOp::LE: {
                                if (const_cond1) {
                                    operand1 = get_asm_reg(cond2);
                                    operand2 = new IR2asm::Operand2(const_cond1->get_value());
                                } else {
                                    operand1 = get_asm_reg(cond1);
                                    if (const_cond2) {
                                        operand2 = new IR2asm::Operand2(const_cond2->get_value());
                                    } else {
                                        operand2 = new IR2asm::Operand2(*get_asm_reg(cond2));
                                    }
                                }
                                code += IR2asm::cmp(operand1, operand2);
                                if (const_cond1) {
                                    code += IR2asm::bgt(bb_label[true_bb]);
                                } else {
                                    code += IR2asm::ble(bb_label[true_bb]);
                                }
                                code += IR2asm::b(bb_label[false_bb]);
                            }
                            break;
                            default:
                                break;
                        }
                    }
                    break;
                    case Instruction::muladd:
                        break;
                    case Instruction::mulsub:
                        break;
                    case Instruction::asradd:
                        break;
                    case Instruction::lsladd:
                        break;
                    case Instruction::lsradd:
                        break;
                    case Instruction::asrsub:
                        break;
                    case Instruction::lslsub:
                        break;
                    case Instruction::lsrsub:
                        break;
                    case Instruction::smul_lo:
                        break;
                    case Instruction::smul_hi:
                        break;
                    case Instruction::load_const_offset: {
                        auto load_const_offset_inst = dynamic_cast<LoadConstOffsetInst*>(inst);
                        auto ptr = load_const_offset_inst->get_lval();
                        auto offset = load_const_offset_inst->get_offset()->get_value();
                        auto arg_ptr = dynamic_cast<Argument*>(ptr);
                        if (arg_ptr) {
                            auto arg_num = arg_ptr->get_arg_no();
                            if (arg_num > 4) {
                                code += IR2asm::getelementptr(get_asm_reg(inst), arg_on_stack[arg_num]);
                                code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), offset));
                            } else {
                                if (get_asm_reg(ptr)->get_id() < 0) {
                                    code += IR2asm::getelementptr(get_asm_reg(inst), stack_map[ptr]);
                                    code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), offset));
                                } else {
                                    code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(ptr), offset));
                                }
                            }
                        } else {
                            if (get_asm_reg(ptr)->get_id() < 0) {
                                code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), stack_map[ptr]->get_offset() + offset));
                            } else {
                                code += IR2asm::load(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(ptr), offset));
                            }
                        }
                    }
                    break;
                    case Instruction::store_const_offset: {
                        auto store_const_offset = dynamic_cast<LoadConstOffsetInst*>(inst);
                        auto ptr = store_const_offset->get_lval();
                        auto offset = store_const_offset->get_offset()->get_value();
                        auto arg_ptr = dynamic_cast<Argument*>(ptr);
                        if (arg_ptr) {
                            auto arg_num = arg_ptr->get_arg_no();
                            if (arg_num > 4) {
                                code += IR2asm::getelementptr(get_asm_reg(inst), arg_on_stack[arg_num]);
                                code += IR2asm::store(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), offset));
                            } else {
                                if (get_asm_reg(ptr)->get_id() < 0) {
                                    code += IR2asm::getelementptr(get_asm_reg(inst), stack_map[ptr]);
                                    code += IR2asm::store(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), offset));
                                } else {
                                    code += IR2asm::store(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(ptr), offset));
                                }
                            }
                        } else {
                            if (get_asm_reg(ptr)->get_id() < 0) {
                                code += IR2asm::store(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(inst), stack_map[ptr]->get_offset() + offset));
                            } else {
                                code += IR2asm::store(get_asm_reg(inst), new IR2asm::Regbase(*get_asm_reg(ptr), offset));
                            }
                        }
                    }
                    break;
                    case Instruction::mov_const: {
                        auto mov_inst = dynamic_cast<MovConstInst*>(inst);
                        auto const_val = mov_inst->get_const()->get_value();
                        code += IR2asm::ldr_const(get_asm_reg(inst), new IR2asm::constant(const_val));
                    }
                    break;
                    default:
                        break;
        }
        return code;
    }
//     } // namespace CodeGen