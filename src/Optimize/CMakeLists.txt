add_library(
        pass STATIC
        Pass.cpp
        CFGSymplifiler.cpp
        DominateTree.cpp
        mem2reg.cpp
        LIR.cpp
        ActiveVar.cpp
        ConstPropagation.cpp
        DeadCodeElimination.cpp
        CFG_analyse.cpp
        AvailableExpr.cpp
)
