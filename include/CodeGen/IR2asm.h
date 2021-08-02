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


    // RegAllocDriver* reg_alloc;
    // Module* m_;
    // Function* func_;

    enum CmpOp {
    EQ, // ==
    NE, // !=
    GT, // >
    GE, // >=
    LT, // <
    LE, // <=
    NOP
    };

    std::string ldr_const(Reg* rd, constant *val);
    std::string mov(Reg* rd, Operand2 *opr2);
    std::string getelementptr(Reg* rd, Location * ptr);
    std::string cond(CmpOp & cop);
    std::string ret();
    std::string ret(Value* retval);
    std::string ret(Location *addr);
    std::string beq(Location *addr);
    std::string bne(Location *addr);
    std::string bgt(Location *addr);
    std::string bge(Location *addr);
    std::string blt(Location *addr);
    std::string ble(Location *addr);
    std::string b(Location* label);
    std::string br(Location* label);
    std::string cbz(Reg* rs, Location* label);
    std::string cbnz(Reg* rs, Location* label);
    std::string br(Reg* cond, Location* success, Location* fail);
    std::string cmp(Reg* rs, Operand2* opr2);
    std::string add(Reg* rd, Reg* rs, Operand2* opr2);
    std::string sub(Reg* rd, Reg* rs, Operand2* opr2);
    std::string r_sub(Reg* rd, Reg* rs, Operand2* opr2);
    std::string mul(Reg* rd, Reg* rs, Operand2* opr2);
    std::string sdiv(Reg* rd, Reg* rs, Operand2* opr2);
    std::string srem(Reg* rd, Reg* rs, Operand2* opr2);
    std::string load(Reg* rd, Location* addr);
    std::string store(Reg* rs, Location* addr);
    std::string call(label* fun);
    
    std::string land(Reg* rd, Reg* rs, Operand2* opr2);
    std::string lor(Reg* rd, Reg* rs, Operand2* opr2);
    std::string lxor(Reg* rd, Reg* rs, Operand2* opr2);

    std::string asr(Reg* rd, Reg* rs, Operand2* opr2);
    std::string lsl(Reg* rd, Reg* rs, Operand2* opr2);
    std::string lsr(Reg* rd, Reg* rs, Operand2* opr2);

    std::string muladd(Reg* rd, Reg* rs, Reg* rt, Reg* rn);
    std::string mulsub(Reg* rd, Reg* rs, Reg* rt, Reg* rn);
    std::string smul(Reg* rd1, Reg* rd2, Reg* rs, Reg* rt);
}

#endif