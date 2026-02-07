#include "poApp.h"
#include "poPE.h"
#include "poCompiler.h"
#include "poCOFF.h"
#include "poELF.h"
#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>

using namespace po;

#ifdef WIN32
static bool openLibraryFile(const std::string& fileName, poCommonObjectFileFormat& coff)
{
    const std::vector<std::string> searchPaths = {
        "c:\\Program Files (x86)\\Windows Kits\\10\\Lib",
        "c:\\Program Files\\Windows Kits\\10\\Lib"
    };

    std::string targetDir;
    for (const std::string& path : searchPaths)
    {
        std::filesystem::path dir(path);
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
        {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir))
        {
            if (entry.is_directory())
            {
                const std::string fullPath = entry.path().string() + "\\um\\x64\\" + fileName;
                if (!std::filesystem::exists(fullPath))
                {
                    continue;
                }

                /* Choose the most recent version of the windows kits */
                if (targetDir.empty())
                {
                    targetDir = fullPath;
                }
                else if (fullPath > targetDir)
                {
                    targetDir = fullPath;
                }
            }
        }
    }

    if (coff.open(targetDir))
    {
        std::cout << "Loaded " << targetDir << std::endl;
        return true;
    }

    std::cout << "Failed to open " << fileName << std::endl;
    return false;
}
#else
static bool openLibraryFile(const std::string& fileName, poELF& elf)
{
    std::filesystem::path dir("/usr/lib");
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
    {
        std::cout << "Failed to open " << fileName << std::endl;
        return false;
    }

    int version = -1;
    std::filesystem::path path;
    for (const std::filesystem::path& file : std::filesystem::directory_iterator(dir))
    {
        const std::string& candidate = file.filename().string();
        const std::string& substring = candidate.substr(0, fileName.size());

        if (substring == fileName)
        {
            if (version == -1 &&
                file.extension().string() == ".so")
            {
                path = file;
                version = 0;
            }
            else
            {
                const std::string& extension = file.extension().string();
                if (extension.size() >= 2)
                {
                    const int newVersion = std::atoi(extension.substr(1).c_str());
                    if (newVersion > version)
                    {
                        version = newVersion;
                        path = file;
                    }
                }
            }
        }
    }

    if (!path.empty())
    {
        if (elf.open(path.string()))
        {
            std::cout << "Loaded " << path.string() << std::endl;
            return true;
        }
    }

    return false;
}
#endif

static int align(const int size, const int alignment)
{
    if (size == 0)
    {
        return alignment;
    }

    const int remainder = size % alignment;
    if (remainder == 0)
    {
        return size;
    }
    return size + (alignment - remainder);
}

int main(const int numArgs, const char** const args)
{
    std::cout << "Pora Compiler" << std::endl;
    
    if (numArgs <= 1)
    {
        return 0;
    }

    const std::string option = args[1];

    if (option == "build")
    {
#if WIN32
        // Link the Windows libraries
        poCommonObjectFileFormat kernel32;
        if (!openLibraryFile("kernel32.lib", kernel32))
        {
            std::cout << "Failed to open kernel32.lib" << std::endl;
            return 0;
        }

        poCommonObjectFileFormat user32;
        if (!openLibraryFile("user32.lib", user32))
        {
            std::cout << "Failed to open user32.lib" << std::endl;
            return 0;
        }
#else
        // Link the C runtime
        poELF libc;
        if (!openLibraryFile("libc.so", libc))
        {
            std::cout << "Failed to open libc.so" << std::endl;
            return 0;
        }
#endif

        poCompiler compiler;

        for (int i = 2; i < numArgs; i++)
        {
            const std::string arg = args[i];
            if (arg == "/dump")
            {
                compiler.setDebugDump(true);
            }
            else if (arg == "/O0")
            {
                // No optimizations
                compiler.setOptimizationLevel(OPTIMIZATION_LEVEL_0);
            }
            else if (arg == "/O1")
            {
                compiler.setOptimizationLevel(OPTIMIZATION_LEVEL_1);
            }
            else if (arg == "/O2")
            {
                compiler.setOptimizationLevel(OPTIMIZATION_LEVEL_2);
            }
            else
            {
                std::cout << "Compiling " << arg << std::endl;
                compiler.addFile(arg);
            }
        }

        if (compiler.compile() == 0)
        {
            for (auto& error : compiler.errors())
            {
                std::cout << error << std::endl;
            }
            return 0;
        }

        if (compiler.assembler().entryPoint() == -1)
        {
            std::cout << "No entry point defined." << std::endl;
            return 0;
        }

        const std::vector<unsigned char>& programData = compiler.assembler().programData();
        const std::vector<unsigned char>& initializedData = compiler.assembler().initializedData();
        const std::vector<unsigned char>& readOnlyData = compiler.assembler().readOnlyData();
        const std::vector<unsigned char>& pltData = compiler.assembler().pltData();
        const std::vector<unsigned char>& pltgotData = compiler.assembler().pltGotData();
        std::unordered_map<std::string, int>& imports = compiler.assembler().imports();
        const std::vector<poRelocation>& pltRelocations = compiler.assembler().pltRelocations();

        // Build the import tables
#ifdef WIN32
        // Create portable executable
        poPortableExecutable exe;
        
        const int kernel32ImportTable = exe.addImportTable("Kernel32.dll");
        const int user32ImportTable = exe.addImportTable("User32.dll");

        for (const poImport& import : kernel32.imports())
        {
            if (imports.find(import.importName()) != imports.end())
            {
                // TODO: we adding duplicate entries?
                poPortableExecutableImportTable& table = exe.importTable(kernel32ImportTable);
                table.addImport(import.importName(), import.importOrdinal());
            }
        }

        for (const poImport& import : user32.imports())
        {
            if (imports.find(import.importName()) != imports.end())
            {
                // TODO: we adding duplicate entries?
                poPortableExecutableImportTable& table = exe.importTable(user32ImportTable);
                table.addImport(import.importName(), import.importOrdinal());
            }
        }
#else
        // Create ELF executable file
        poELF elf;

        elf.addLibrary("libc.so.6");

        std::unordered_set<std::string> mapped;
        for (const poELF_Symbol& symbol : libc.symbols())
        {
            if (imports.find(symbol.getName()) != imports.end() &&
                    mapped.find(symbol.getName()) == mapped.end())
            {
                elf.addSymbol(symbol.getName(), symbol.id());
                mapped.insert(symbol.getName());
            }
        }

        for (const poRelocation& rel : pltRelocations)
        {
            elf.addPltRelocation(rel.relocationPos(), rel.symbol());
        }

        for (const poELF_Symbol& symbol : libc.symbols())
        {
            const auto& it = imports.find(symbol.getName());
            if (it != imports.end())
            {
                it->second = symbol.addr();
            }
        }
        
#endif

        // Final linking..
#ifdef WIN32
         // Create the data sections
        exe.setEntryPoint(compiler.assembler().entryPoint());
        exe.addSection(poSectionType::TEXT, align(int(programData.size()), 1024));
        exe.addSection(poSectionType::INITIALIZED, align(int(initializedData.size()), 1024));
        exe.addSection(poSectionType::UNINITIALIZED, 1024);
        exe.addSection(poSectionType::IDATA, 1024 * 2);
        exe.addSection(poSectionType::READONLY, align(int(readOnlyData.size()), 1024));
        exe.initializeSections();
 
        for (poImportEntry& importEntry : exe.importTable(kernel32ImportTable).imports())
        {
            const auto& it = imports.find(importEntry.name());
            if (it != imports.end())
            {
                it->second = importEntry.addressRVA();
            }
        }

        for (poImportEntry& importEntry : exe.importTable(user32ImportTable).imports())
        {
            const auto& it = imports.find(importEntry.name());
            if (it != imports.end())
            {
                it->second = importEntry.addressRVA();
            }
        }
       
        compiler.assembler().link(0x1000, exe.initializedDataImagePos(), exe.readonlyDataImagePos(), 0, 0);

        // Write program data
        std::memcpy(exe.textSection().data().data(), programData.data(), programData.size());
        std::memcpy(exe.initializedDataSection().data().data(), initializedData.data(), initializedData.size());
        std::memcpy(exe.readOnlyDataSection().data().data(), readOnlyData.data(), readOnlyData.size());

        // Write the executable file
        exe.write("app.exe");

        std::cout << "Program compiled successfully: app.exe" << std::endl;
#else
        elf.setEntryPoint(compiler.assembler().entryPoint());
        elf.add(poELF_SectionType::SHT_NULL, "", 0); // always start with a NULL section
        elf.add(poELF_SectionType::SHT_PROGBITS, ".interp", 0);
        elf.add(poELF_SectionType::SHT_STRTAB, ".shstrtab", align(int(programData.size()), 1024));
        elf.add(poELF_SectionType::SHT_STRTAB, ".dynstr", 0);
        elf.add(poELF_SectionType::SHT_PROGBITS, ".text", align(int(programData.size()), 1024));
        elf.add(poELF_SectionType::SHT_PROGBITS, ".rodata", align(int(readOnlyData.size()), 1024));
        elf.add(poELF_SectionType::SHT_PROGBITS, ".data", align(int(initializedData.size()), 1024));
        elf.add(poELF_SectionType::SHT_HASH, ".hash", 0);
        elf.add(poELF_SectionType::SHT_SYMTAB, ".symtab", 0);
        //elf.add(poELF_SectionType::SHT_RELA, ".rela.text", 0);
        //elf.add(poELF_SectionType::SHT_REL, ".rel.text", 0);
        elf.add(poELF_SectionType::SHT_PROGBITS, ".plt", align(int(pltData.size()), 1024));
        elf.add(poELF_SectionType::SHT_PROGBITS, ".got.plt", align(int(pltgotData.size()), 1024));
        elf.add(poELF_SectionType::SHT_RELA, ".rela.plt", 0);
        //elf.add(poELF_SectionType::SHT_NOBITS, ".tbss", 1024);
        //elf.add(poELF_SectionType::SHT_PROGBITS, ".tdata", 1024);
        elf.add(poELF_SectionType::SHT_DYNAMIC, ".dynamic", 0); // always the last section
        elf.initializeSections();

        compiler.assembler().link(
                elf.textSection().virtualAddress(),
                elf.initializedDataSection().virtualAddress(),
                elf.readOnlySection().virtualAddress(),
                elf.pltDataSection().virtualAddress(),
                elf.pltGotDataSection().virtualAddress());

        // Write program data
        std::memcpy(elf.textSection().data().data(), programData.data(), programData.size());
        std::memcpy(elf.readOnlySection().data().data(), readOnlyData.data(), readOnlyData.size());
        std::memcpy(elf.initializedDataSection().data().data(), initializedData.data(), initializedData.size());
        std::memcpy(elf.pltDataSection().data().data(), pltData.data(), pltData.size());
        std::memcpy(elf.pltGotDataSection().data().data(), pltgotData.data(), pltgotData.size());

        // Write the elf file
        elf.write("app");

        std::cout << "Program compiled successfully: app" << std::endl;
#endif
    }

    return 0;
}
