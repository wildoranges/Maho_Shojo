//
// Created by cjb on 7/25/21.
//

#ifndef MHSJ_VALUEGEN_H
#define MHSJ_VALUEGEN_H

#include <string>
#include <set>

namespace IR2asm {

    const int max_reg = 15;

    const std::string reg_name[] = {"r0", "r1", "r2", "r3", "r4", "r5",
                "r6", "r7", "r8", "r9", "r10", "r11",
                "r12", "sp", "lr", "pc"};

    class Value {
        public:
        virtual bool is_reg() = 0;
        virtual bool is_const() = 0;
        virtual std::string get_code() = 0;
    };

    class Location{
        public:
        virtual std::string get_code() = 0;
    };


    class Reg : public Value {
    public:
        int id;

        Reg(int i) : id(i) {}

        int get_id() { return id; }

        bool is_reg() final {return true;}
        bool is_const() final {return false;}
        std::string get_code(){ return reg_name[id]; }
    };

    

    class Regbase: public Location{
        private:
            Reg reg_;
            int offset;

        public:
            Regbase(Reg reg, int offset): reg_(reg), offset(offset){}
            Reg &get_reg(){return reg_;}
            int get_offset(){return offset;}
            void set_offset(int x){offset = x;}
            std::string get_code(){
                if(!offset)return "[" + reg_.get_code() + "]";
                else {
                    return "[" + reg_.get_code() + ", #" + std::to_string(offset) + "]";
                }
            }
    };

    class label: public Location{
        private:
            std::string label_;
            int offset = 0;
        
        public:
            explicit label(std::string labl):label_(labl){}
            explicit label(std::string labl, int offst):label_(labl), offset(offst){}
            std::string get_label(){return label_;}
            std::string get_code(){
                if(!offset)return label_;
                return label_ + std::to_string(offset);
            }
    };
    class constant: public Value{
        private:
            int value_;

        public:
            explicit constant(int val):value_(val){}
            ~constant(){}
            bool is_const() final {return true;}
            bool is_reg() final {return false;}
            int get_val(){return value_;}
            std::string get_code(){return "#" + std::to_string(value_);}
    };

enum ShiftOp{
    asr,
    lsl,
    lsr
};
    class Operand2: public Value{
        private:
            Reg reg_;
            ShiftOp shift_op_;
            int value_;

        public:
            explicit Operand2(Reg reg, ShiftOp shift_op, int val):reg_(reg), shift_op_(shift_op), value_(val){}
            explicit Operand2(Reg reg):reg_(reg){}
            ~Operand2(){}
            bool is_const() final {return false;}
            bool is_reg() final {return false;}
            std::string get_operand2(ShiftOp shift_op) {if (shift_op == ShiftOp::asr) return "ASR";
                                                        else if (shift_op == ShiftOp::lsl) return "LSL";
                                                        else if (shift_op == ShiftOp::lsr) return "LSR";
                                                        else return "ERROR";}
            std::string get_code(){if (!shift_op_) return reg_.get_code();
                                    else return reg_.get_code() + " " + get_operand2(shift_op_) + " " + "#" + std::to_string(value_);}
    };

}

#endif //MHSJ_VALUEGEN_H