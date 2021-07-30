#include<CodeGen.h>
#include<RegAlloc.h>

namespace CodeGen{

    std::string global(std::string name){
        return IR2asm::space + ".globl" + name + IR2asm::endl;
    }

    bool iszeroinit(Constant * init){
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
    std::string global_def_gen(Module* module){
        std::string code;
        for(auto var: module->get_global_variable()){
            std::string name = var->get_name();
            bool isconst = var->is_const();
            auto initializer = var->get_init();
            bool isinitialized = (dynamic_cast<ConstantZero *>(initializer) != nullptr);
            bool isarray = (dynamic_cast<ConstantArray *>(initializer) != nullptr);
            int size = var->get_type()->get_size();
            code += IR2asm::space;
            code += ".type" + name + ", %object" + IR2asm::endl;
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
        }
        return code;
    }

    int stack_space_allocation(Function* fun){
        //TODO: stack allocation to alloca and reg spill
        //fill in stack_map
        int size = 0;
        stack_map.clear();
        for(auto iter: reg_map){
            Value* vreg = iter.first;
            Interval* interval = iter.second;
            if(interval->reg_num > 0){
                used_reg.insert(interval->reg_num);
                continue;
            }
            int type_size = vreg->get_type()->get_size();
            size += type_size;
            stack_map.insert({vreg, new IR2asm::Regbase(IR2asm::frame_ptr, -size)});
        }
        for(auto inst: fun->get_entry_block()->get_instructions()){
            auto alloc = dynamic_cast<AllocaInst*>(inst);
            if(!alloc)continue;
            int type_size = alloc->get_alloca_type()->get_size();
            size += type_size;
            stack_map.insert({dynamic_cast<Value *>(alloc), new IR2asm::Regbase(IR2asm::frame_ptr, -size)});
        }
        return size;
    }

    std::string callee_reg_store(Function* fun){
        //TODO
        std::string code;
        code += IR2asm::space;
        code += "push {";
        for(auto reg: used_reg){
            code += IR2asm::reg_name[reg];
            code += ", ";
        }
        code += (IR2asm::frame_ptr).get_code();
        code += ", ";
        code += "lr}";
        code += IR2asm::endl;
        return code;
    }

    std::string callee_reg_restore(Function* fun){
        //TODO
        std::string code;
        code += IR2asm::space;
        code += "pop {";
        for(auto reg: used_reg){
            code += IR2asm::reg_name[reg];
            code += ", ";
        }
        code += (IR2asm::frame_ptr).get_code();
        code += ", ";
        code += "pc}";
        code += IR2asm::endl;
        return code;
    }

    std::string callee_stack_operation_in(Function* fun, int stack_size){
        //TODO
        std::string code;
        code += IR2asm::space;
        code += "add ";
        code += (IR2asm::frame_ptr).get_code();
        code += ", sp, #";
        code += std::to_string(2 * int_size);
        code += IR2asm::endl;
        code += IR2asm::space;
        code += "sub sp, sp, #";
        code += std::to_string(stack_size);
        code += IR2asm::endl;
        return code;
    }

    std::string callee_stack_operation_out(Function* fun, int stack_size){
        //TODO
        std::string code;
        code += IR2asm::space;
        code += "sub sp, ";
        code += IR2asm::frame_ptr.get_code();
        code += ", #";
        code += std::to_string(2 * int_size);
        code += IR2asm::endl;
        return code;
    }

    std::string caller_reg_store(Function* fun){
        //TODO
    }

    std::string caller_reg_restore(Function* fun){
        //TODO
    }

    void make_global_table(Module* module){
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
    
    std::string print_global_table(){
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

    std::string module_gen(Module* module){
        std::string code;
        code += global_def_gen(module);
        RegAllocDriver driver = RegAllocDriver(module);
        driver.compute_reg_alloc();
        //TODO: function definition
        make_global_table(module);
        func_no = 0;
        for(auto func_: module->get_functions()){
            reg_map = driver.get_reg_alloc_in_func(func_);
            code += function_gen(func_);
            func_no++;
        }
        //TODO: static data segmentation
        //TODO: *other machine infomation
    }

    void make_linear_bb(Function* fun){
        //TODO:sort bb and make bb label, put in CodeGen::bb_label
        //TODO: label gen, name mangling as bbx_y for yth bb in function no.x .
        bb_label.clear();
        linear_bb.clear();
        bb_no = 0;
        for(auto bb: fun->get_basic_blocks()){
            if(bb != fun->get_entry_block()){
                std::string label_str = "bb" + std::to_string(func_no) + "_" + std::to_string(bb_no);
                IR2asm::label newlabel = IR2asm::label(label_str);
                bb_label.insert({bb, newlabel});
            }
            else{
                bb_label.insert({bb, IR2asm::label(fun->get_name())});
            }
            linear_bb.push_back(bb);
            bb_no++;
        }
        return;
    }

    void global_label_gen(Function* fun){
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

    std::string function_gen(Function* fun){
        std::string code;
        global_label_gen(fun);
        make_linear_bb(fun);
        int stack_size = stack_space_allocation(fun);
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

    std::string bb_gen(BasicBlock* bb){
        std::string code;
        //TODO: instruction gen
        return code;
    }

    std::string instr_gen(Instruction * inst){
        std::string code;
        //TODO: call functions in IR2asm , deal with phi inst(mov inst)
        return code;
    }
} // namespace CodeGen
