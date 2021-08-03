//
// Created by cjb on 7/23/21.
//

#ifndef MHSJ_REG_ALLOC_H
#define MHSJ_REG_ALLOC_H

#include "Value.h"
#include "Module.h"
#include "ValueGen.h"
#include <map>
#include <set>
#include <queue>


//const std::set<int> param_reg_id = {0,1,2,3};
//const std::set<int> ret_reg_id = {0};


class Interval;
class RegAlloc;


struct Range{
    Range(int f,int t):from(f),to(t){}
    int from;
    int to;
};


class Interval{
public:
    explicit Interval(Value* value):val(value){}
    int reg_num = -1;
    Value* val;
    //int assigned_reg;
    std::list<Range*> range_list;
    std::list<int> position_list;
    //Interval* parent;
    //std::list<Interval*> children;
    void add_range(int from,int to);
    void add_use_pos(int pos){position_list.push_front(pos);}
    //Interval* split(int id);
    bool covers(int id);
    bool covers(Instruction* inst);
    //bool covers(int from,int to);
    bool intersects(Interval* interval);
    void union_interval(Interval* interval);
    //Interval* child_at(int id);
};


struct cmp_interval{
    bool operator()(const Interval* a, const Interval* b) const {
        auto a_from = (*(a->range_list.begin()))->from;
        auto b_from = (*(b->range_list.begin()))->from;
        if(a_from!=b_from){
            return a_from < b_from;
        }else{
            return a->val->get_name() < b->val->get_name();
        }
    }
};

const int priority[] = {
        12,//r0
        11,//r1
        10,//r2
        9,//r3
        8,//r4
        7,//r5
        6,//r6
        5,//r7
        4,//r8
        3,//r9
        2,//r10
        -1,//r11
        1//r12
};


struct cmp_reg {
    bool operator()(const int reg1,const int reg2)const{
        assert(reg1>=0&&reg1<=12&&reg2<=12&&reg2>=0&&"invalid reg id");
        return priority[reg1] > priority[reg2];
    }
};

//const std::set<int> general_reg_id = {10,9,8,7,6,5,4};
//const std::set<int> func_reg_id = {3,2,1,0,12};
const std::vector<int> all_reg_id = {0,1,2,3,4,5,6,7,8,9,10,12};

class RegAllocDriver{
public:
    explicit RegAllocDriver(Module* m):module(m){}
    void compute_reg_alloc();
    std::map<Value*, Interval*>& get_reg_alloc_in_func(Function* f){return reg_alloc[f];}
private:
    std::map<Function*,std::map<Value*,Interval*>> reg_alloc;
    Module* module;
};

class RegAlloc{
public:
    explicit RegAlloc(Function* f):func(f){}
    //int get_reg(Value* value);
    void execute();
    void compute_block_order();
    void number_operations();
    void build_intervals();
    void union_phi_val();
    void walk_intervals();
    void set_unused_reg_num();
    std::map<Value*,Interval*>& get_reg_alloc(){return val2Inter;}
private:
    void get_dfs_order(BasicBlock* bb,std::set<BasicBlock*>& visited);
    void add_interval(Interval* interval){interval_list.insert(interval);}
    void add_reg_to_pool(int reg_id);
    bool try_alloc_free_reg();
    std::set<int> unused_reg_id = {all_reg_id.begin(),all_reg_id.end()};
    //std::priority_queue<int,std::vector<int>,cmp_reg> remained_general_reg_id = {general_reg_id.begin(),general_reg_id.end()};
    //std::priority_queue<int,std::vector<int>,cmp_reg> remained_func_reg_id = {func_reg_id.begin(),func_reg_id.end()};
    std::set<int,cmp_reg> remained_all_reg_id = {all_reg_id.begin(),all_reg_id.end()};
    std::set<Interval *> active = {};
    Interval* current = nullptr;
    std::map<Value*, Interval*> val2Inter;
    Function* func;
    std::list<BasicBlock*> block_order={};
    std::set<Interval*,cmp_interval> interval_list;
};

#endif //MHSJ_REG_ALLOC_H
