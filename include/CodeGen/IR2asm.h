#ifndef MHSJ_IR2ASM_H
#define MHSJ_IR2ASM_H

#include<RegAlloc.h>
#include<ValueGen.h>
#include<Instruction.h>
#include<Module.h>
#include<LIR.h>


namespace IR2asm{
    const std::string space = std::string(4, ' ');
    const std::string endl = "\n";

    Reg frame_ptr = Reg(11);
    Reg sp = Reg(13);
    Reg lr = Reg(14);
    Reg pc = Reg(15);

    RegAllocDriver* reg_alloc;
    Module* m_;
    Function* func_;

    enum CmpOp {
    EQ, // ==
    NE, // !=
    GT, // >
    GE, // >=
    LT, // <
    LE, // <=
    NOP
    };

    std::string getelementptr(Location * ptr);
    std::string cond(CmpOp & cop);
    std::string ret();
    std::string ret(Value* retval);
    std::string br(Value* label);
    std::string br(Value* cond, Value* success, Value* fail);
    std::string add(Value* rd, Value* rs, Value* opr2);
    std::string sub(Value* rd, Value* rs, Value* opr2);
    std::string mul(Value* rd, Value* rs, Value* opr2);
    std::string sdiv(Value* rd, Value* rs, Value* opr2);
    std::string srem(Value* rd, Value* rs, Value* opr2);
    std::string load(Value* rd, Value* addr);
    std::string store(Value* rs, Value* addr);
    std::string call(label* fun);
    
    std::string land(Value* rd, Value* rs, Value* opr2);
    std::string lor(Value* rd, Value* rs, Value* opr2);
    std::string lxor(Value* rd, Value* rs, Value* opr2);

    std::string asr(Value* rd, Value* rs, Value* opr2);
    std::string lsl(Value* rd, Value* rs, Value* opr2);
    std::string lsr(Value* rd, Value* rs, Value* opr2);

    std::string muladd(Value* rd, Value* rs, Value* rt, Value* rn);
    std::string mulsub(Value* rd, Value* rs, Value* rt, Value* rn);
    std::string smul(Value* rd1, Value* rd2, Value* rs, Value* rt);
};

#endif