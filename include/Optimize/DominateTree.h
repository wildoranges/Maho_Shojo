//
// Created by cjb on 7/20/21.
//

#ifndef MHSJ_DOMINATETREE_H
#define MHSJ_DOMINATETREE_H

#include <list>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include "Pass.h"
#include "BasicBlock.h"
#include "Module.h"

//class DomNode;

//using DomNodePtr = std::shared_ptr<DomNode>;

class DominateTree : public Pass{
public:
    explicit DominateTree(Module* module): Pass(module){}
    void execute()final;
    void get_revserse_post_order(Function* f);
    void get_post_order(BasicBlock* bb,std::set<BasicBlock*>& visited);
    void get_bb_idom(Function* f);
    void get_bb_dom_front(Function* f);
    BasicBlock* intersect(BasicBlock* b1, BasicBlock* b2);

private:
    std::list<BasicBlock*> reverse_post_order;
    std::map<BasicBlock*,int> bb2int;
    std::vector<BasicBlock*> doms;
};

#endif //MHSJ_DOMINATETREE_H
