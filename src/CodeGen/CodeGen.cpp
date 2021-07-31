#include<CodeGen.h>
#include<RegAlloc.h>
#include<stack>

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

    //TODO: global varibale
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
            for(auto iter: *reg_map){
                Value* vreg = iter.first;
                Interval* interval = iter.second;
                if(interval->reg_num > 0){
                    if(interval->reg_num > 3){
                        used_reg.second.insert(interval->reg_num);
                    }
                    else{
                        used_reg.first.insert(interval->reg_num);
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
                size += reg_size;
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
            for(auto iter: *reg_map){
                Value* vreg = iter.first;
                Interval* interval = iter.second;
                if(interval->reg_num > 0){
                    if(interval->reg_num > 3){
                        used_reg.second.insert(interval->reg_num);
                    }
                    else{
                        used_reg.first.insert(interval->reg_num);
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
        int reg_store_size = reg_size * (used_reg.second.size() + (have_func_call)? 1 : 0 );
        for(auto item: arg_on_stack){
            int offset = item->get_offset();
            item->set_offset(offset + reg_store_size + size);
        }
        return size;
    }

    std::string CodeGen::callee_reg_store(Function* fun){
        //TODO
        std::string code;
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
        //TODO
        std::string code;
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
        //TODO
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
        //TODO
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

    std::string CodeGen::caller_reg_store(Function* fun){
        //TODO
        std::string code;
        int arg_num = fun->get_num_of_args();
        if(arg_num == 0)return "";
        if(arg_num > 4)arg_num = 4;
        code += IR2asm::space;
        code += "push {";
        int i = 0;
        for(; i < arg_num - 1; i++){
            if(used_reg.first.find(i) == used_reg.first.end())continue;
            code += IR2asm::Reg(arg_num).get_code();
            code += ", ";
        }
        code += IR2asm::Reg(arg_num).get_code();
        code += "}";
        code += IR2asm::endl;
        return code;
    }

    std::string CodeGen::caller_reg_restore(Function* fun){
        //TODO
        std::string code;
        int arg_num = fun->get_num_of_args();
        if(arg_num == 0)return "";
        if(arg_num > 4)arg_num = 4;
        code += IR2asm::space;
        code += "pop {";
        int i = 0;
        for(; i < arg_num - 1; i++){
            if(used_reg.first.find(i) == used_reg.first.end())continue;
            code += IR2asm::Reg(arg_num).get_code();
            code += ", ";
        }
        code += IR2asm::Reg(arg_num).get_code();
        code += "}";
        code += IR2asm::endl;
        return code;
    }

    void CodeGen::make_global_table(Module* module){
        //TODO:global var use analysis
        for(auto var: module->get_global_variable()){
            for(auto use: var->get_use_list()){
                Function* func_;
                func_ = dynamic_cast<Instruction *>(use.val_)->get_parent()->get_parent();
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
        //TODO
        std::string code;
        for(auto iter: global_variable_table){
            GlobalVariable* var = iter.first;
            IR2asm::label label = iter.second;
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
        code += global_def_gen(module);
        std::cout << code;
        RegAllocDriver driver = RegAllocDriver(module);
        driver.compute_reg_alloc();
        //TODO: function definition
        make_global_table(module);
        func_no = 0;
        for(auto func_: module->get_functions()){
            reg_map = &driver.get_reg_alloc_in_func(func_);
            code += function_gen(func_);
            func_no++;
        }
        //TODO: static data segmentation
        //TODO: *other machine infomation
        return code;
    }

    void CodeGen::make_linear_bb(Function* fun){
        //TODO:sort bb and make bb label, put in CodeGen::bb_label
        //TODO: label gen, name mangling as bbx_y for yth bb in function no.x .
        bb_label.clear();
        linear_bb.clear();
        bb_no = 0;
        for(auto bb: fun->get_basic_blocks()){
            if(bb != fun->get_entry_block()){
                std::string label_str = "bb" + std::to_string(func_no) + "_" + std::to_string(bb_no);
                IR2asm::label *newlabel = new IR2asm::label(label_str);
                bb_label.insert({bb, newlabel});
            }
            else{
                bb_label.insert({bb, new IR2asm::label(fun->get_name())});
            }
            linear_bb.push_back(bb);
            bb_no++;
        }
        return;
    }

    void CodeGen::global_label_gen(Function* fun){
        //TODO: global varibal address store after program(.LCPIx_y), fill in CodeGen::global_variable_table
        auto used_global = global_variable_use.find(fun)->second;
        global_variable_table.clear();
        label_no = 0;
        for(auto var: used_global){
            std::string label_str = "Addr" + std::to_string(func_no) + "_" + std::to_string(label_no);
            IR2asm::label new_label = IR2asm::label(label_str);
            label_no++;
            global_variable_table.insert({var, new_label});
        }
    }

    void CodeGen::func_call_check(Function* fun){
        max_arg_size = 0;
        for(auto bb: fun->get_basic_blocks()){
            for(auto inst: bb->get_instructions()){
                auto call = dynamic_cast<CallInst*>(inst);
                if(!call)continue;
                int arg_size = 0;
                for(auto arg: call->get_function()->get_args()){
                    arg_size += arg->get_type()->get_size();
                }
                if(arg_size > max_arg_size)max_arg_size = arg_size;
                have_func_call = true;
                return;
            }
        }
        have_func_call = false;
        return;
    }

    std::string CodeGen::arg_move(CallInst* call){
        //TODO: arg on stack in reversed sequence
        std::string regcode;
        std::string memcode;
        std::stack<Value *> push_stack;
        auto fun = dynamic_cast<Function *>(call->get_operand(0));
        int i = 0;
        for(auto arg: call->get_operands()){
            if(dynamic_cast<Function *>(arg))continue;
            if(i < 4){
                regcode += IR2asm::space;
                auto reg = (*reg_map).find(arg)->second->reg_num;
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
                    regcode += stack_map.find(arg)->second->get_code();
                    regcode += IR2asm::endl;
                }
            }
            else{
                push_stack.push(arg);
            }
            i++;
        }
        while(!push_stack.empty()){
            Value* arg = push_stack.top();
            push_stack.pop();
            auto reg = (*reg_map).find(arg)->second->reg_num;
            if(reg >= 0){
                memcode += IR2asm::space;
                memcode += "push {";
                memcode += IR2asm::Reg(reg).get_code();
                memcode += "}";
                memcode += IR2asm::endl;
            }
            else{
                auto srcaddr = stack_map.find(arg)->second;
                memcode += IR2asm::space;
                memcode += "ldr r0, ";
                memcode += srcaddr->get_code();
                memcode += IR2asm::endl;
                memcode += IR2asm::space;
                memcode += "push {r0}";
                memcode += IR2asm::endl;
            }
        }
        return memcode + regcode;
    }

    std::string CodeGen::function_gen(Function* fun){
        std::string code;
        global_label_gen(fun);
        make_linear_bb(fun);
        func_call_check(fun);
        int stack_size = stack_space_allocation(fun) + max_arg_size;
        code += fun->get_name() + ":" + IR2asm::endl;
        code += callee_reg_store(fun);
        code += callee_stack_operation_in(fun, stack_size);

        //TODO: basicblock gen
        for(auto bb: linear_bb){
            code += bb_gen(bb);
        }
        code += callee_stack_operation_out(fun, stack_size);
        code += callee_reg_restore(fun);
        code += print_global_table();
        return code;
    }

    std::string CodeGen::bb_gen(BasicBlock* bb){
        std::string code;
        code += bb_label[bb]->get_code()+":"+IR2asm::endl;
        for(auto inst : bb->get_instructions()){
            code += instr_gen(inst);
        }
        //TODO: instruction gen
        return code;
    }

    std::string CodeGen::instr_gen(Instruction * inst){
        std::string code;
        //TODO: call functions in IR2asm , deal with phi inst(mov inst)
        auto instr_type = inst->get_instr_type();
        switch (instr_type)
        {
        case Instruction::ret:
            if (inst->is_void()) {
                code += IR2asm::ret();
            } else {
                auto ret_val = inst->get_operand(0);
                auto const_ret_val = dynamic_cast<ConstantInt*>(ret_val);
                if (const_ret_val) {
                    code += IR2asm::ret(get_asm_const(const_ret_val));
                } else {
                    code += IR2asm::ret(get_asm_reg(ret_val));
                }
            }
            break;
        case Instruction::br:
            if (inst->get_num_operand() == 1) {
                code += IR2asm::br(bb_label[dynamic_cast<BasicBlock*>(inst->get_operand(0))]);
            } else if (inst->get_num_operand() == 3) {
                auto cond = dynamic_cast<CmpInst*>(inst->get_operand(0));
                auto true_bb = dynamic_cast<BasicBlock*>(inst->get_operand(1));
                auto false_bb = dynamic_cast<BasicBlock*>(inst->get_operand(2));
                auto cmp_op = cond->get_cmp_op();
                if (cmp_op == CmpInst::EQ) code += IR2asm::br(get_asm_reg(inst->get_operand(0)), bb_label[false_bb], bb_label[true_bb]);
                else code += IR2asm::br(get_asm_reg(inst->get_operand(0)), bb_label[true_bb], bb_label[false_bb]);
            }
            break;
        case Instruction::add:
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
            break;
        case Instruction::sub:
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
            break;
        case Instruction::mul:
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
            break;
        case Instruction::sdiv: // divide consant can be optimized
            auto op1 = inst->get_operand(0);
            auto op2 = inst->get_operand(1);
            auto operand1 = op1;
            auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
            code += IR2asm::sdiv(get_asm_reg(inst), get_asm_reg(operand1), operand2);
            break;
        case Instruction::srem: // srem -> sdiv and msub
            break;
        case Instruction::alloca:   // has done before
            break;
        case Instruction::load:
            IR2asm::Location *addr;
            auto global_addr = dynamic_cast<GlobalVariable*>(inst->get_operand(0));
            if (global_addr) {
                addr = &global_variable_table[global_addr];
            } else {
                addr = stack_map[inst->get_operand(0)];
            }
            code += IR2asm::load(get_asm_reg(inst), addr);
            break;
        case Instruction::store:
            IR2asm::Location *addr;
            auto global_addr = dynamic_cast<GlobalVariable*>(inst->get_operand(1));
            if (global_addr) {
                addr = &global_variable_table[global_addr];
            } else {
                addr = stack_map[inst->get_operand(0)];
            }
            code += IR2asm::load(get_asm_reg(inst->get_operand(0)), addr);
            break;
        case Instruction::cmp:
            auto cmp_inst = dynamic_cast<CmpInst*>(inst);
            auto cmp_op = cmp_inst->get_cmp_op();
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
            if (cmp_op == CmpInst::EQ || cmp_op == CmpInst::NE) {
                code += IR2asm::lxor(get_asm_reg(inst), get_asm_reg(operand1), operand2);
            } else if (cmp_op == CmpInst::GT || cmp_op == CmpInst::GE || cmp_op == CmpInst::LT || cmp_op == CmpInst::LE) {
                code += IR2asm::cmp(get_asm_reg(operand1), operand2);
                code += IR2asm::mov(get_asm_reg(inst), new IR2asm::Operand2(0));
                switch (cmp_op)
                {
                case CmpInst::GT:
                    if (const_op1) code += IR2asm::movle(get_asm_reg(inst), new IR2asm::Operand2(1));
                    else code += IR2asm::movgt(get_asm_reg(inst), new IR2asm::Operand2(1));
                    break;
                case CmpInst::GE:
                    if (const_op1) code += IR2asm::movlt(get_asm_reg(inst), new IR2asm::Operand2(1));
                    else code += IR2asm::movge(get_asm_reg(inst), new IR2asm::Operand2(1));
                    break;
                case CmpInst::LT:
                    if (const_op1) code += IR2asm::movge(get_asm_reg(inst), new IR2asm::Operand2(1));
                    else code += IR2asm::movlt(get_asm_reg(inst), new IR2asm::Operand2(1));
                    break;
                case CmpInst::LE:
                    if (const_op1) code += IR2asm::movgt(get_asm_reg(inst), new IR2asm::Operand2(1));
                    else code += IR2asm::movle(get_asm_reg(inst), new IR2asm::Operand2(1));
                    break;
                default:
                    break;
                }
            }
            break;
        case Instruction::phi:  // has done before
            break;
        case Instruction::call:
            code += IR2asm::call(new IR2asm::label(inst->get_operand(0)->get_name()));
            break;
        case Instruction::getelementptr:
            IR2asm::Location *addr;
            auto global_addr = dynamic_cast<GlobalVariable*>(inst->get_operand(0));
            if (global_addr) {
                addr = &global_variable_table[global_addr];
            } else {
                addr = stack_map[inst->get_operand(0)];
            }
            code += IR2asm::getelementptr(get_asm_reg(inst), addr);
            break;
        case Instruction::land:
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
            break;
        case Instruction::lor:
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
            break;
        case Instruction::lxor:
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
            break;
        case Instruction::zext:
            break;
        case Instruction::asr:
            auto op1 = inst->get_operand(0);
            auto op2 = inst->get_operand(1);
            auto operand1 = op1;
            auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
            code += IR2asm::asr(get_asm_reg(inst), get_asm_reg(operand1), operand2);
            break;
        case Instruction::lsl:
            auto op1 = inst->get_operand(0);
            auto op2 = inst->get_operand(1);
            auto operand1 = op1;
            auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
            code += IR2asm::lsl(get_asm_reg(inst), get_asm_reg(operand1), operand2);
            break;
        case Instruction::lsr:
            auto op1 = inst->get_operand(0);
            auto op2 = inst->get_operand(1);
            auto operand1 = op1;
            auto operand2 = new IR2asm::Operand2(*get_asm_reg(op2));
            code += IR2asm::lsr(get_asm_reg(inst), get_asm_reg(operand1), operand2);
            break;
        case Instruction::cmpbr:
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
        default:
            break;
        }
        return code;
    }
// } // namespace CodeGen
