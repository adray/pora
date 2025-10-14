# Pora Core

cmake_minimum_required (VERSION 3.8)

set (PORACORE_SOURCES
    "poAST.h"
    "poAST.cpp"
    "poCFG.h"
    "poCFG.cpp"
    "poCOFF.h"
    "poCOFF.cpp"
    "poDom.h"
    "poDom.cpp"
    "poPE.h"
    "poPE.cpp"
    "poPhiWeb.h"
    "poPhiWeb.cpp"
    "poLex.h"
    "poLex.cpp"
    "poLive.h"
    "poLive.cpp"
    "poOptFold.h"
    "poOptFold.cpp"
    "poOptMemToReg.h"
    "poOptMemToReg.cpp"
    "poParser.h"
    "poParser.cpp"
    "poTypeChecker.h"
    "poTypeChecker.cpp"
    "poModule.h"
    "poModule.cpp"
    "poSSA.h"
    "poSSA.cpp"
    "poRegGraph.h"
    "poRegGraph.cpp"
    "poRegLinear.h"
    "poRegLinear.cpp"
    "poUses.h"
    "poUses.cpp"
    "poNLF.h"
    "poNLF.cpp"
    "poType.h"
    "poType.cpp"
    "poTypeResolver.h"
    "poTypeResolver.cpp"
    "poOptDCE.cpp"
    "poOptDCE.h"
    "poOptCopy.cpp"
    "poOptCopy.h"
)

project ("poracore")

# Add source to this project's executable.
add_library(poracore STATIC ${PORACORE_SOURCES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET poracore PROPERTY CXX_STANDARD 20)
endif()



