#include<CodeGen.h>

namespace CodeGen{
    const int int_align = 4;
    const int int_size = 4;
    const int int_p2align = 2;

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

    void stack_space_allocation(Function* fun){
        //TODO: stack allocation to alloca and reg spill
        //fill in stack_map
    }

    std::string callee_reg_store(Function* fun){
        //TODO
    }

    std::string callee_reg_restore(Function* fun){
        //TODO
    }

    std::string callee_stack_operation_in(Function* fun){
        //TODO
    }

    std::string callee_stack_operation_out(Function* fun){
        //TODO
    }

    std::string caller_reg_store(Function* fun){
        //TODO
    }

    std::string caller_reg_restore(Function* fun){
        //TODO
    }

    void make_global_table(Function *fun){
        //TODO: global varibal address store after program(.LCPIx_y), fill in CodeGen::global_variable_table
        //TODO
    }
    
    std::string print_global_table(Function* fun){
        //TODO
    }

    std::string module_gen(Module* module){
        std::string code;
        code += global_def_gen(module);
        //TODO: function definition
        for(auto func_: module->get_functions()){
            code += function_gen(func_);
        }
        //TODO: static data segmentation
        //TODO: *other machine infomation
    }

    void make_linear_bb(Function* fun){
        //TODO:sort bb and make bb label, put in CodeGen::bb_label

        //TODO: label gen, name mangling as bbx_y for yth bb in function no.x .
    }

    std::string function_gen(Function* fun){
        std::string code;

        make_global_table(fun);
        code += callee_stack_operation_in(fun);
        code += callee_reg_store(fun);
        //TODO: basicblock gen
        for(auto bb: linear_bb){
            code += bb_gen(bb);
        }
        code += callee_reg_restore(fun);
        code += callee_stack_operation_out(fun);
        code += print_global_table(fun);
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
