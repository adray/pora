#include "poELF.h"

#include <fstream>

using namespace po;

#define EI_NIDENT 16
typedef uint16_t    Elf64_Half;
typedef uint32_t    Elf64_Word;
typedef uint64_t    Elf64_Addr;
typedef uint64_t    Elf64_Off;
typedef uint64_t    Elf64_Xword;

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
