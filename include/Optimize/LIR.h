//
// Created by cjb on 7/23/21.
//

#ifndef MHSJS_LIR_H
#define MHSJS_LIR_H

#include "Module.h"
#include "Pass.h"

class LIR:public Pass
{
public:
    explicit LIR(Module* module): Pass(module){}
    void execute() final;
};

#endif //MHSJS_LIR_H
