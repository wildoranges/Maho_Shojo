//
// Created by cjb on 7/25/21.
//

#ifndef MHSJ_VALUEGEN_H
#define MHSJ_VALUEGEN_H

#include <string>
#include <set>

namespace CodeGen {

    const int max_reg = 15;

    const std::string reg_name[] = {"r0", "r1", "r2", "r3", "r4", "r5",
                "r6", "r7", "r8", "r9", "r10", "r11",
                "r12", "sp", "lr", "pc"};

    class value {
        virtual bool is_reg() = 0;
    };


    class reg : public value {
    public:
        int id;

        reg(int i) : id(i) {}

        int get_id() { return id; }

        bool is_reg() final {return true;}
        const std::string& get_name() { return reg_name[id]; }
    };

}

#endif //MHSJ_VALUEGEN_H