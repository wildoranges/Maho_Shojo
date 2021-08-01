#ifndef SYSY_INSTRUCTION_H
#define SYSY_INSTRUCTION_H

#include "User.h"
#include "Type.h"
#include "Constant.h"
#include "BasicBlock.h"

class BasicBlock;
class Function;

class Instruction : public User
{
public:
    enum OpID
    {
        // Terminator Instructions
        ret,
        br,
        // Standard binary operators
        add,
        sub,
        mul,
        sdiv,
        srem,
        // Memory operators
        alloca,
        load,
        store,
        // Other operators
        cmp,
        phi,
        call,
        getelementptr, 
        // Logical operators
        land,
        lor,
        lxor,
        // Zero extend
        zext,
        // Shift operators
        asr,
        lsl,
        lsr,
        // LIR operators
        cmpbr,
        muladd,
        mulsub,
        asradd,
        lsladd,
        lsradd,
        asrsub,
        lslsub,
        lsrsub,
        smul_lo,
        smul_hi,
        load_const_offset,
        store_const_offset,

    };
    // create instruction, auto insert to bb
    // ty here is result type
    Instruction(Type *ty, OpID id, unsigned num_ops,
                BasicBlock *parent);
    Instruction(Type *ty, OpID id, unsigned num_ops);
    inline const BasicBlock *get_parent() const { return parent_; }
    inline BasicBlock *get_parent() { return parent_; }
    void set_parent(BasicBlock *parent) { this->parent_ = parent; }
    // Return the function this instruction belongs to.
    Function *get_function();
    Module *get_module();

    OpID get_instr_type() { return op_id_; }
    std::string get_instr_op_name() {
        switch (op_id_)
        {
            case ret: return "ret"; break;
            case br: return "br"; break;
            case add: return "add"; break;
            case sub: return "sub"; break;
            case mul: return "mul"; break;
            case sdiv: return "sdiv"; break;
            case srem: return "srem"; break;
            case land: return "and"; break;
            case lor: return "or"; break;
            case lxor: return "xor"; break;
            case alloca: return "alloca"; break;
            case load: return "load"; break;
            case store: return "store"; break;
            case cmp: return "cmp"; break;
            case cmpbr: return "cmpbr"; break;
            case phi: return "phi"; break;
            case call: return "call"; break;
            case getelementptr: return "getelementptr"; break;
            case zext: return "zext"; break;
            case asr: return "asr"; break;
            case lsl: return "lsl"; break;
            case lsr: return "lsr"; break;
            case muladd: return "muladd"; break;
            case mulsub: return "mulsub"; break;
            case asradd: return "asradd"; break;
            case lsladd: return "lsladd"; break;
            case lsradd: return "lsradd"; break;
            case asrsub: return "asrsub"; break;
            case lslsub: return "lslsub"; break;
            case lsrsub: return "lsrsub"; break;
            case smul_lo: return "smul_lo"; break;
            case smul_hi: return "smul_hi"; break;
            case load_const_offset: return "load_const_offset"; break;
            case store_const_offset: return "store_const_offset"; break;
        
        default: return ""; break;
        }
    }



    bool is_void() { return ((op_id_ == cmpbr) || (op_id_ == ret) || (op_id_ == br) || (op_id_ == store_const_offset) || (op_id_ == store) || (op_id_ == call && this->get_type()->is_void_type())); }

    bool is_phi() { return op_id_ == phi; }
    bool is_store() { return op_id_ == store; }
    bool is_alloca() { return op_id_ == alloca; }
    bool is_ret() { return op_id_ == ret; }
    bool is_load() { return op_id_ == load; }
    bool is_br() { return op_id_ == br; }

    bool is_load_const_offset() { return op_id_ == load_const_offset; }
    bool is_store_const_offset() { return op_id_ == store_const_offset; }

    bool is_add() { return op_id_ == add; }
    bool is_sub() { return op_id_ == sub; }
    bool is_mul() { return op_id_ == mul; }
    bool is_div() { return op_id_ == sdiv; }
    bool is_rem() { return op_id_ == srem; }

    bool is_smul_lo() { return op_id_ == smul_lo; }
    bool is_smul_hi() { return op_id_ == smul_hi; }

    bool is_muladd() { return op_id_ == muladd; }
    bool is_mulsub() { return op_id_ == mulsub; }

    bool is_and() { return op_id_ == land; }
    bool is_or() { return op_id_ == lor; }
    bool is_xor() { return op_id_ == lxor; }

    bool is_asr() { return op_id_ == asr; }
    bool is_lsl() { return op_id_ == lsl; }
    bool is_lsr() { return op_id_ == lsr; }

    bool is_asradd() { return op_id_ == asradd; }
    bool is_lsladd() { return op_id_ == lsladd; }
    bool is_lsradd() { return op_id_ == lsradd; }
    bool is_asrsub() { return op_id_ == asrsub; }
    bool is_lslsub() { return op_id_ == lslsub; }
    bool is_lsrsub() { return op_id_ == lsrsub; }

    bool is_cmp() { return op_id_ == cmp; }
    bool is_cmpbr() { return op_id_ == cmpbr; }

    bool is_call() { return op_id_ == call; }
    bool is_gep() { return op_id_ == getelementptr; }
    bool is_zext() { return op_id_ == zext; }

    bool isShiftBinary()
    {
        return (is_asradd() || is_lsladd() || is_lsradd() || 
                is_asrsub() || is_lslsub() || is_lsrsub()) && 
                (get_num_operand() == 3);
    }

    bool isBinary()
    {
        return (is_add() || is_sub() || is_mul() || is_div() || is_rem() || 
                is_and() || is_or() || is_xor() || 
                is_asr() || is_lsl() || is_lsr()) && 
                (get_num_operand() == 2);
    }

    bool isTerminator() { return is_br() || is_ret() || is_cmpbr(); }

    void set_id(int id){id_ = id;}
    int get_id() const{return id_;}

    virtual Instruction *copy_inst(BasicBlock *BB)  = 0;

private:
    BasicBlock *parent_;
    OpID op_id_;
    int id_;
    unsigned num_ops_;
};

class BinaryInst : public Instruction
{
private:
    BinaryInst(Type *ty, OpID id, Value *v1, Value *v2,
               BasicBlock *bb);

public:
    static BinaryInst *create_add(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_sub(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_mul(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_sdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_srem(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_and(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_or(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_xor(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_asr(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_lsl(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_lsr(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_smul_lo(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_smul_hi(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new BinaryInst(get_type(),get_instr_type(),get_operand(0),get_operand(1),BB);
    }

private:
    void assertValid();
};

class MulAddInst : public Instruction
{
private:
    MulAddInst(Type *ty, OpID id, Value *v1, Value *v2, Value *v3,
               BasicBlock *bb);

public:
    static MulAddInst *create_muladd(Value *v1, Value *v2, Value *v3, BasicBlock *bb, Module *m);

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new MulAddInst(get_type(),get_instr_type(),get_operand(0),get_operand(1),get_operand(2),BB);
    }

private:
    void assertValid();
};

class MulSubInst : public Instruction
{
private:
    MulSubInst(Type *ty, OpID id, Value *v1, Value *v2, Value *v3,
               BasicBlock *bb);

public:
    static MulSubInst *create_mulsub(Value *v1, Value *v2, Value *v3, BasicBlock *bb, Module *m);

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new MulSubInst(get_type(),get_instr_type(),get_operand(0),get_operand(1),get_operand(2),BB);
    }

private:
    void assertValid();
};

class ShiftBinaryInst : public Instruction
{
private:
    ShiftBinaryInst(Type *ty, OpID id, Value *v1, Value *v2, Constant *v3,
               BasicBlock *bb);

public:
    static ShiftBinaryInst *create_asradd(Value *v1, Value *v2, Constant *v3, BasicBlock *bb, Module *m);
    static ShiftBinaryInst *create_lsladd(Value *v1, Value *v2, Constant *v3, BasicBlock *bb, Module *m);
    static ShiftBinaryInst *create_lsradd(Value *v1, Value *v2, Constant *v3, BasicBlock *bb, Module *m);
    static ShiftBinaryInst *create_asrsub(Value *v1, Value *v2, Constant *v3, BasicBlock *bb, Module *m);
    static ShiftBinaryInst *create_lslsub(Value *v1, Value *v2, Constant *v3, BasicBlock *bb, Module *m);
    static ShiftBinaryInst *create_lsrsub(Value *v1, Value *v2, Constant *v3, BasicBlock *bb, Module *m);

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new ShiftBinaryInst(get_type(),get_instr_type(),get_operand(0),get_operand(1),v3_,BB);
    }

private:
    Constant *v3_;

    void assertValid();
};

class CmpInst : public Instruction
{
public:
    enum CmpOp
    {
        EQ, // ==
        NE, // !=
        GT, // >
        GE, // >=
        LT, // <
        LE  // <=
    };

private:
    CmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs,
            BasicBlock *bb);

public:
    static CmpInst *create_cmp(CmpOp op, Value *lhs, Value *rhs,
                               BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new CmpInst(get_type(),cmp_op_,get_operand(0),get_operand(1),BB);
    }

private:
    CmpOp cmp_op_;

    void assertValid();
};

class CmpBrInst : public Instruction
{
public:
    using CmpOp = CmpInst::CmpOp;

private:
    CmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false,
            BasicBlock *bb);

public:
    static CmpBrInst *create_cmpbr(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false,
                               BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    bool is_cmp_br() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new CmpBrInst(cmp_op_,get_operand(0),get_operand(1),true_BB_,false_BB_,BB);
    }

private:
    CmpOp cmp_op_;
    BasicBlock* true_BB_;
    BasicBlock* false_BB_;

    void assertValid();
};

class CallInst : public Instruction
{
protected:
    CallInst(Function *func, std::vector<Value *> args, BasicBlock *bb);

public:
    static CallInst *create(Function *func, std::vector<Value *> args, BasicBlock *bb);
    FunctionType *get_function_type() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new CallInst(get_function(),args_,BB);
    }

private:
    std::vector<Value *> args_;
};

class BranchInst : public Instruction
{
private:
    BranchInst(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
               BasicBlock *bb);
    BranchInst(BasicBlock *if_true, BasicBlock *bb);

public:
    static BranchInst *create_cond_br(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
                                      BasicBlock *bb);
    static BranchInst *create_br(BasicBlock *if_true, BasicBlock *bb);

    bool is_cond_br() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        if (get_num_operand() == 1){
            return new BranchInst(true_BB_,BB);
        }
        else{
            return new BranchInst(get_operand(0),true_BB_,false_BB_,BB);
        }
    }

private:
    BasicBlock* true_BB_;
    BasicBlock* false_BB_;

};

class ReturnInst : public Instruction
{
private:
    ReturnInst(Value *val, BasicBlock *bb);
    ReturnInst(BasicBlock *bb);

public:
    static ReturnInst *create_ret(Value *val, BasicBlock *bb);
    static ReturnInst *create_void_ret(BasicBlock *bb);
    bool is_void_ret() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        if (is_void_ret()){
            return new ReturnInst(BB);
        }
        else{
            return new ReturnInst(get_operand(0),BB);
        }
    }

};

class GetElementPtrInst : public Instruction
{
private:
    GetElementPtrInst(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);

public:
    static Type *get_element_type(Value *ptr, std::vector<Value *> idxs);
    static GetElementPtrInst *create_gep(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);
    Type *get_element_type() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new GetElementPtrInst(get_operand(0),idxs_,BB);
    }

private:
    Type *element_ty_;
    std::vector<Value *> idxs_;
};

class StoreInst : public Instruction
{
private:
    StoreInst(Value *val, Value *ptr, BasicBlock *bb);

public:
    static StoreInst *create_store(Value *val, Value *ptr, BasicBlock *bb);

    Value *get_rval() { return this->get_operand(0); }
    Value *get_lval() { return this->get_operand(1); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new StoreInst(get_operand(0),get_operand(1),BB);
    }

};

class StoreConstOffsetInst : public Instruction
{
private:
    StoreConstOffsetInst(Value *val, Value *ptr, ConstantInt *offset, BasicBlock *bb);

public:
    static StoreConstOffsetInst *create_store_const_offset(Value *val, Value *ptr, ConstantInt *offset, BasicBlock *bb);

    Value *get_rval() { return this->get_operand(0); }
    Value *get_lval() { return this->get_operand(1); }
    ConstantInt *get_offset() { return dynamic_cast<ConstantInt*>(this->get_operand(2)); }

    virtual std::string print() override;

    // TODO: copy function

};

class LoadInst : public Instruction
{
private:
    LoadInst(Type *ty, Value *ptr, BasicBlock *bb);

public:
    static LoadInst *create_load(Type *ty, Value *ptr, BasicBlock *bb);
    Value *get_lval() { return this->get_operand(0); }

    Type *get_load_type() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new LoadInst(get_type(),get_operand(0),BB);
    }

};

class LoadConstOffsetInst : public Instruction
{
private:
    LoadConstOffsetInst(Type *ty, Value *ptr, ConstantInt *offset, BasicBlock *bb);

public:
    static LoadConstOffsetInst *create_load_const_offset(Type *ty, Value *ptr, ConstantInt *offset, BasicBlock *bb);
    Value *get_lval() { return this->get_operand(0); }
    ConstantInt *get_offset() { return dynamic_cast<ConstantInt*>(this->get_operand(1)); }

    Type *get_load_type() const;

    virtual std::string print() override;

    // TODO: copy function

};

class AllocaInst : public Instruction
{
private:
    AllocaInst(Type *ty, BasicBlock *bb);

public:
    static AllocaInst *create_alloca(Type *ty, BasicBlock *bb);

    Type *get_alloca_type() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new AllocaInst(alloca_ty_,BB);
    }

private:
    Type *alloca_ty_;
};

class ZextInst : public Instruction
{
private:
    ZextInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

public:
    static ZextInst *create_zext(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return new ZextInst(get_instr_type(),get_operand(0),dest_ty_,BB);
    }

private:
    Type *dest_ty_;
};

class PhiInst : public Instruction
{
private:
    PhiInst(OpID op, std::vector<Value *> vals, std::vector<BasicBlock *> val_bbs, Type *ty, BasicBlock *bb);
    PhiInst(Type *ty, OpID op, unsigned num_ops, BasicBlock *bb)
        : Instruction(ty, op, num_ops, bb) {}
    Value *l_val_;

public:
    static PhiInst *create_phi(Type *ty, BasicBlock *bb);
    Value *get_lval() { return l_val_; }
    void set_lval(Value *l_val) { l_val_ = l_val; }
    void add_phi_pair_operand(Value *val, Value *pre_bb)
    {
        this->add_operand(val);
        this->add_operand(pre_bb);
    }
    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *BB) override final{
        return nullptr;
    }

};

#endif // SYSY_INSTRUCTION_H
