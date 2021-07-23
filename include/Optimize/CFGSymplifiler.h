//
// Created by cjb on 7/20/21.
//

#ifndef MHSJCFGSYMPLIFILER_H
#define MHSJCFGSYMPLIFILER_H

#include "Pass.h"
#include "Module.h"

class CFGSymplifiler :public Pass{
public:
    explicit CFGSymplifiler(Module *m) : Pass(m) {}
    virtual void execute() override;
};

#endif //MHSJCFGSYMPLIFILER_H
