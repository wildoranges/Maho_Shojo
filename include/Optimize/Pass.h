//
// Created by cjb on 7/20/21.
//

#ifndef MHSJ_PASS_H
#define MHSJ_PASS_H

#include <string>
#include <list>
#include "Module.h"



class Pass{
public:
    explicit Pass(Module* m){module = m;}
    virtual void execute() = 0;
    std::string name;
protected:
    Module* module;
};

template<typename T>
using PassList = std::list<T*>;

class PassMgr{
public:
    explicit PassMgr(Module* m){module = m;pass_list = PassList<Pass>();}
    void register_pass(){
        //TODO:Finish Pass Register
    };
private:
    Module* module;
    PassList<Pass> pass_list;
};

#endif //MHSJ_PASS_H
