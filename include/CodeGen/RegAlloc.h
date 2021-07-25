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


const std::set<int> param_reg_id = {0,1,2,3};
const std::set<int> ret_reg_id = {0};

//const std::set<CodeGen::reg> param_regs = {CodeGen::reg(0),CodeGen::reg(1),
//                                           CodeGen::reg(2),CodeGen::reg(3)};
//
//const std::set<CodeGen::reg> ret_regs = {CodeGen::reg(0)};

class Interval;
class RegAlloc;


struct Range{
    int from;
    int to;
};

//struct cmp_range{
//    bool operator()(Range* a, Range* b){
//        return a->from > b->from;
//    }
//};

class Interval{
public:
    explicit Interval(Value* value):val(value){}
    int reg_num;
    Value* val;
    int assigned_reg;
    std::list<Range*> range_list;
    //std::priority_queue<Range*,std::list<Range*>,cmp_range> range_list;
    std::list<int> position_list;
    Interval* parent;
    std::list<Interval*> children;
    void add_range(int from,int to);
    void add_use_pos(int pos);
    Interval* split(int id);
    bool covers(int id);
    bool intersects(Interval* interval);
    Interval* child_at(int id);
};


struct cmp_interval{
    bool operator()(Interval* a,Interval* b){
        return (*a->range_list.begin())->from > (*b->range_list.begin())->from;
    }
};


class RegAlloc{
public:
    RegAlloc(Module* m):module(m){}
    int get_reg(Value* value);
    void compute_block_order(Function* f);
    void get_dfs_order(BasicBlock* bb,std::set<BasicBlock*>& visited);
    void number_operations();
    //void compute_local_live_sets();
    void build_intervals();
    void walk_intervals();
    void add_interval(Interval* interval){interval_list.push(interval);}
    bool try_alloc_free_reg();
    void try_alloc_blocked_reg();
private:
    std::set<int> remained_reg_id = {12,11,10,9,8,7,6,5,4,3,2,1,0};
    std::set<Interval *> active = {};
    std::set<Interval *> inactive = {};
    std::set<Interval *> handled = {};
    Interval* current = nullptr;
    std::map<Value*, Interval*> val2Inter;
    Module* module;
    //std::map<Value*,int> val2reg;
    std::list<BasicBlock*> block_order;
    std::priority_queue<Interval*,std::list<Interval*>,cmp_interval> interval_list;
};

#endif //MHSJ_REG_ALLOC_H
