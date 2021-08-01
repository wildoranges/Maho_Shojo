#include<IR2asm.h>

namespace IR2asm{

// std::string immopr(Value* imm){
//     std::string asmstr;
//     return "#" + dynamic_cast<ConstantInt *>(imm)->get_value();
// }

// std::string reg(Value* vreg){
//     auto reg_map = (reg_alloc)->get_reg_alloc_in_func(func_);
//     auto reg_no = reg_map[vreg]->reg_num;
//     return reg_name[reg_no];
// }

// std::string op2(Value* singleopr){
//     auto imm = dynamic_cast<Constant *>(singleopr);
//     if(imm){
//         return immopr(imm);
//     }
//     else{
//         return reg(singleopr); 
//     }
// }

//TODO:zext

std::string mov(Reg* rd, Operand2 *opr2) {
    std::string asmstr;
    asmstr += space;
    asmstr += "mov";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string movgt(Reg* rd, Operand2 *opr2) {
    std::string asmstr;
    asmstr += space;
    asmstr += "movgt";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string movge(Reg* rd, Operand2 *opr2) {
    std::string asmstr;
    asmstr += space;
    asmstr += "movge";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string movlt(Reg* rd, Operand2 *opr2) {
    std::string asmstr;
    asmstr += space;
    asmstr += "movlt";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string movle(Reg* rd, Operand2 *opr2) {
    std::string asmstr;
    asmstr += space;
    asmstr += "movle";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string getelementptr(Reg* rd, Location * ptr){
    std::string asmstr;
    asmstr += space;
    asmstr += "ldr ";
    asmstr += rd->get_code();
    asmstr += ", ";
    auto regbase = dynamic_cast<Regbase *>(ptr);
    if(regbase){
        asmstr += regbase->get_reg().get_code();
        asmstr += endl;
        asmstr += space;
        int offset = regbase->get_offset();
        asmstr += (offset > 0)?"add ":"sub ";
        asmstr += rd->get_code();
        asmstr += ", ";
        asmstr += rd->get_code();
        asmstr += ", #";
        asmstr += std::to_string(abs(regbase->get_offset()));
    }
    else{
        auto labelexpr = dynamic_cast<label *>(ptr);
        asmstr += "=";
        asmstr += labelexpr->get_code();
    }
    asmstr += endl;
    return asmstr;
}

std::string call(label* fun){
    std::string asmstr;
    asmstr += space;
    asmstr += "bl ";
    asmstr += fun->get_code();
    asmstr += endl;
    return asmstr;
}

std::string ret(Value* retval){
    std::string asmstr;
    asmstr += space;
    asmstr += "mov r0, ";
    asmstr += retval->get_code();
    asmstr += endl;
    // asmstr += space;
    // asmstr += "br lr" + endl;
    return asmstr;
}

std::string ret(){
    std::string asmstr;
    asmstr += space;
    asmstr += "br lr" + endl;
    return asmstr;
}

std::string br(Location* label) {
    return b(label);
}

// TODO: can be simplified
std::string br(Reg* cond, Location* success, Location* fail) {
    return cbnz(cond, success) + cbz(cond, fail);
}

std::string b(Location *addr) {
    std::string asmstr;
    asmstr += space;
    asmstr += "b ";
    asmstr += addr->get_code();
    asmstr += endl;
    return asmstr;
}

std::string cbz(Reg* rs, Location *addr) {
    std::string asmstr;
    asmstr += space;
    asmstr += "cbz ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += addr->get_code();
    asmstr += endl;
    return asmstr;
}

std::string cbnz(Reg* rs, Location *addr) {
    std::string asmstr;
    asmstr += space;
    asmstr += "cbnz ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += addr->get_code();
    asmstr += endl;
    return asmstr;
}

std::string cmp(Reg* rs, Operand2* opr2) {
    std::string asmstr;
    asmstr += space;
    asmstr += "cmp ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string add(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "add ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string sub(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "sub ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string r_sub(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "rsb ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string mul(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "mul ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string sdiv(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "sdiv ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string srem(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "srem ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string land(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "and ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string lor(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "or ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string lxor(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "eor ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string asr(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "asr ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string lsl(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "lsl ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string lsr(Reg* rd, Reg* rs, Operand2* opr2){
    std::string asmstr;
    asmstr += space;
    asmstr += "lsr ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += opr2->get_code();
    asmstr += endl;
    return asmstr;
}

std::string load(Reg* rd, Location* addr){
    std::string asmstr;
    asmstr += space;
    asmstr += "ldr ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += addr->get_code();
    asmstr += endl;
}

std::string store(Reg* rd, Location* addr){
    std::string asmstr;
    asmstr += space;
    asmstr += "str ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += addr->get_code();
    asmstr += endl;
}

std::string muladd(Reg* rd, Reg* rs, Reg* rt, Reg* rn){
    std::string asmstr;
    asmstr += space;
    asmstr += "mla ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += rt->get_code();
    asmstr += ", ";
    asmstr += rn->get_code();
    asmstr += endl;
    return asmstr;
}

std::string mulsub(Reg* rd, Reg* rs, Reg* rt, Reg* rn){
    std::string asmstr;
    asmstr += space;
    asmstr += "mls ";
    asmstr += rd->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += rt->get_code();
    asmstr += ", ";
    asmstr += rn->get_code();
    asmstr += endl;
    return asmstr;
}

std::string smul(Reg* rd1, Reg* rd2, Reg* rs, Reg* rt){
    std::string asmstr;
    asmstr += space;
    asmstr += "mls ";
    asmstr += rd1->get_code();
    asmstr += ", ";
    asmstr += rd2->get_code();
    asmstr += ", ";
    asmstr += rs->get_code();
    asmstr += ", ";
    asmstr += rt->get_code();
    asmstr += endl;
    return asmstr;
}

//TODO:初值处理

}

