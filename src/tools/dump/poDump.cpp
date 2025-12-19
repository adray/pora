#include "poDump.h"
#include "poCOFF.h"
#include "poPE.h"
#include "poELF.h"

#include <iostream>
#include <cstring>

using namespace po;

int main(const int numArgs, const char** const args)
{
    if (numArgs > 1)
    {
        poPortableExecutable pe;
        poCommonObjectFileFormat coff;
        poELF elf;
        
        if (pe.open(args[1]))
        {
            std::cout << "DLL file opened." << std::endl;
            pe.dumpExports();
        }
        else if (coff.open(args[1]))
        {
            std::cout << "Import LIB opened" << std::endl;
            coff.dump();
        }
        else if (elf.open(args[1]))
        {
            std::cout << "Import LIB opened" << std::endl;
            elf.dump();
        }

        else
        {
            std::cout << "File extension not supported" << std::endl;
        }
    }

    return 0;
}

