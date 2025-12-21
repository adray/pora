#include "poELF.h"

#include <fstream>
#include <iostream>
#include <vector>

using namespace po;

#define EI_NIDENT 16
typedef uint16_t    Elf64_Half;
typedef uint32_t    Elf64_Word;
typedef uint64_t    Elf64_Addr;
typedef uint64_t    Elf64_Off;
typedef uint64_t    Elf64_Xword;
typedef uint16_t    Elf64_Section;

struct ElfHeader64
{
    unsigned char   e_ident[EI_NIDENT];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
};

struct ElfSectionHeader64
{
    Elf64_Word	sh_name;
    Elf64_Word	sh_type;
    Elf64_Xword	sh_flags;
    Elf64_Addr	sh_addr;
    Elf64_Off	sh_offset;
    Elf64_Xword	sh_size;
    Elf64_Word	sh_link;
    Elf64_Word	sh_info;
    Elf64_Xword	sh_addralign;
    Elf64_Xword	sh_entsize;
};

struct ElfProgramHeader64
{
    Elf64_Word	p_type;
    Elf64_Word	p_flags;
    Elf64_Off	p_offset;
    Elf64_Addr	p_vaddr;
    Elf64_Addr	p_paddr;
    Elf64_Xword	p_filesz;
    Elf64_Xword	p_memsz;
    Elf64_Xword	p_align;
};

struct Elf64_Sym
{
    Elf64_Word      st_name;
    unsigned char   st_info;
    unsigned char   st_other;
    Elf64_Section   st_shndx;
    Elf64_Addr      st_value;
    Elf64_Xword     st_size;
};

poELF_Symbol::poELF_Symbol(const int id, const int size, const int strPos)
    :
    _id(id),
    _size(size),
    _strPos(strPos)
{
}

poELF::poELF()
{

}

void poELF::write(const std::string& filename)
{
    std::ofstream stream(filename, std::ios::binary);
    if (stream.is_open())
    {
        ElfHeader64 header = {};
        header.e_ident[0] = 0x7F;
        header.e_ident[1] = 'E';
        header.e_ident[2] = 'L';
        header.e_ident[3] = 'F';
        header.e_ident[4] = 2; // 64 bit
        header.e_ident[5] = 1; // little endian
        header.e_ident[6] = 1; // version 1
        header.e_type = 2; // executable file
        header.e_machine = 0x3E; // x86-64
        header.e_version = 1;
        header.e_entry = 0x400000; // Entry point address
        header.e_phoff = sizeof(ElfHeader64); // Program header table offset
        header.e_shoff = 0; // Section header table offset (we'll set this later)
        header.e_flags = 0;
        header.e_ehsize = sizeof(ElfHeader64);
        header.e_phentsize = 56; // Size of program header entry
        header.e_phnum = 0; // Number of program header entries (we'll set this later)
        header.e_shentsize = 64; // Size of section header entry
        header.e_shnum = 0; // Number of section header entries (we'll set this later)
        header.e_shstrndx = 0; // Section header string table index (we'll set this later)
        stream.write((char*)&header, sizeof(header));
    }
}

bool poELF::open(const std::string& filename)
{
    std::ifstream stream(filename, std::ios::binary);
    if (stream.is_open())
    {
        ElfHeader64 header = {};
        stream.read(reinterpret_cast<char*>( &header ), sizeof(header));
        const size_t programHeaderTableOffset = header.e_phoff - sizeof(header);
        if (header.e_ident[0] == 0x7F &&
            header.e_ident[1] == 'E' &&
            header.e_ident[2] == 'L' &&
            header.e_ident[3] == 'F' &&
            header.e_machine == 0x3E &&
            programHeaderTableOffset == 0)
        {
            std::vector<ElfProgramHeader64> headers;
            for (int i = 0; i < int(header.e_phnum); i++)
            {
                ElfProgramHeader64& programHeader = headers.emplace_back();
                stream.read(reinterpret_cast<char*>(&programHeader), sizeof(programHeader));
            }
            
            const int programDataPos = int(stream.tellg());
            stream.seekg(header.e_shoff);

            size_t dynSymPos = -1;
            size_t dynSymSize = 0;
            std::vector<size_t> stringTablePos; /*list of string tables*/
            std::vector<size_t> stringTableName;
            int shstrtab = -1;

            std::vector<ElfSectionHeader64> sections;
            for (int i = 0; i < int(header.e_shnum); i++)
            {
                ElfSectionHeader64& section = sections.emplace_back();
                stream.read(reinterpret_cast<char*>(&section), sizeof(section));

                if (section.sh_type == 0xB) /* SHT_DYNSYM */
                {
                    dynSymPos = section.sh_offset;
                    dynSymSize = section.sh_size;
                }
                else if (section.sh_type == 0x3) /* SHT_STRTAB */
                {
                    stringTablePos.push_back(section.sh_offset);
                    stringTableName.push_back(section.sh_name);

                    if (i == int(header.e_shstrndx))
                    {
                        shstrtab = int(stringTablePos.size()) - 1;
                    }
                }
            }

            if (dynSymPos != -1) {
                stream.seekg(dynSymPos);
                const size_t num = dynSymSize / sizeof(Elf64_Sym);
                for (size_t i = 0; i < num; i++) {
                    Elf64_Sym sym;
                    stream.read(reinterpret_cast<char*>(&sym), sizeof(sym));

                    _symbols.push_back(poELF_Symbol(
                                int(_symbols.size()),
                                int(sym.st_size),
                                int(sym.st_name)));
                }
            }

            int dynstrtab = -1;
            if (shstrtab != -1)
            {
                for (int i = 0; i < int(stringTablePos.size()); i++) {
                    const int pos = int(stringTablePos[shstrtab]);
                    const int namePos = int(stringTableName[i]);
                    stream.seekg(pos + namePos);
                    
                    std::string name;
                    char c = char(0);
                    stream.read(&c, 1);
                    name += c;
                    while (c != 0) {
                        stream.read(&c, 1);
                        if (c != 0) name += c;
                    }

                    if (name == ".dynstr") {
                        dynstrtab = i;
                    }
                }
            }

            for (poELF_Symbol& symbol : _symbols)
            {
                const int pos = int(stringTablePos[dynstrtab]);
                stream.seekg(pos + symbol.strPos());
                
                std::string name;
                char c = char(0);
                stream.read(&c, 1);
                name += c;
                while (c != 0) {
                    stream.read(&c, 1);
                    if (c != 0) name += c;
                }
                symbol.setName(name);
            }

            return true;
        }
    }

    return false;
}
void poELF::dump()
{
    for (poELF_Symbol& symbol : _symbols)
    {
        std::cout << symbol.id() << " " << symbol.getName() << std::endl;
    }
}

