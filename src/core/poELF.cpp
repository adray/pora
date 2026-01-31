#include "poELF.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <assert.h>

using namespace po;

#define EI_NIDENT 16
typedef uint16_t    Elf64_Half;
typedef uint32_t    Elf64_Word;
typedef uint64_t    Elf64_Addr;
typedef uint64_t    Elf64_Off;
typedef uint64_t    Elf64_Xword;
typedef int64_t    Elf64_Sxword;
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

struct Elf64_Dyn
{
	Elf64_Sxword	d_tag;
   	union
    {
   		Elf64_Xword	d_val;
   		Elf64_Addr	d_ptr;
	};
};


struct Elf64_Rel {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
};

struct Elf64_Rela {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
	Elf64_Sxword	r_addend;
};

// Elf hash function as defined in the Elf documentation
static unsigned int elf_hash(const unsigned char *name)
{
	unsigned long	h = 0, g;
	while (*name)
	{
		h = (h << 4) + *name++;
		if (g = h & 0xf0000000)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}


//
// poELF_Segment
//

poELF_Segment::poELF_Segment(const poELF_SegmentType type, const poELF_SegmentFlags flags)
    :
    _type(type),
    _flags(flags)
{
}

//
// poELF_Section
//

poELF_Section::poELF_Section(const poELF_SectionType& type, const poELF_SectionFlags& flags, const std::string& name, const int size, const int entsize, const bool needsSegment)
    :
    _type(type),
    _flags(flags),
    _name(name),
    _namePos(0),
    _virtualAddress(0),
    _size(size),
    _entsize(entsize),
    _link(0),
    _info(0),
    _needsSegment(needsSegment)
{
    _data.resize(size);
}

//
// poELF_Symbol
//

poELF_Symbol::poELF_Symbol(const int id, const int size, const int strPos)
    :
    _id(id),
    _size(size),
    _strPos(strPos),
    _type(0),
    _addr(0)
{
}

//
// poELF_StringTable
//

poELF_StringTable::poELF_StringTable() {
    _data.push_back(0); // first entry is a NULL entry
}

void poELF_StringTable::write(const std::string& str)
{
    for (char c : str)
    {
        _data.push_back(c);
    }
    _data.push_back(0); 
}

//
// poELF_HashTable
//

poELF_HashTable::poELF_HashTable()
    :
    _numBuckets(-1)
{
    // The first entry is a NULL entry
    auto& entry = _entries.emplace_back();
    entry.hash = 0;
}

void poELF_HashTable::addEntry(const std::string& symbolName)
{
    const int hash = elf_hash((const unsigned char*)(symbolName.c_str()));
    poELF_HashTable::Entry& entry = _entries.emplace_back();
    entry.hash = hash;
    entry.name = symbolName;

    _hash.insert(std::pair<std::string, int>(symbolName, int(_entries.size()-1)));
}

void poELF_HashTable::dump()
{
    std::cout << "Num Buckets: " << _buckets.size() << std::endl;
    std::cout << "Num Chains: " << _chains.size() << std::endl;
    std::cout << "Buckets" << std::endl;
    std::cout << "=========" << std::endl;
    for (int i = 0; i < int(_buckets.size()); i++) {
        std::cout << i << " " << _buckets[i] << std::endl;
    }
    std::cout << "=========" << std::endl;
    std::cout << "Chains" << std::endl;
    std::cout << "=========" << std::endl;
    for (int i = 0; i < int(_chains.size()); i++) {
        std::cout << i << " " << _chains[i] << std::endl;
    }
    std::cout << "=========" << std::endl;
    std::cout << "Symbols" << std::endl;
    std::cout << "=========" << std::endl;
    for (int i = 0; i < int(_symbols.size()); i++) {
        std::cout << i << " " << _entries[_symbols[i]].name << " " << _symbols[i] << std::endl;
    }
    std::cout << "=========" << std::endl;
}

const int poELF_HashTable::lookup(const std::string& symbol)
{
    const auto& it = _hash.find(symbol);
    if (it != _hash.end())
    {
        return it->second;
    }
    return -1;
}
 
void poELF_HashTable::compute()
{
    const int numBuckets = std::min(int(_entries.size()), 5); // TODO: pick a good number
    for (auto& entry : _entries)
    {
        entry.value = entry.hash % numBuckets;
    }

    _buckets.resize(numBuckets);
    _chains.push_back(0); // NULL entry
    _symbols.push_back(0); // NULL entry

    for (int i = 0; i < int(_buckets.size()); i++)
    {
        _buckets[i] = 0;
    }

    for (int i = 1; i < int(_entries.size()); i++)
    {
        auto& entry = _entries[i]; 
        if (_buckets[entry.value] == 0)
        {
            _buckets[entry.value] = i;
            _symbols.push_back(i);
            _chains.push_back(0);
        }
        else
        {
            // Follow the chains
            int id = _buckets[entry.value];
            while (_chains[id] != 0)
            {
                id = _chains[id];
            }
            _chains[id] = i;

            _chains.push_back(0);
            _symbols.push_back(i);
        }
    }

    // Hash table format:
    // =======
    // Num Bucket Entries (I32)
    // Num Chain Entries (I32)
    // =======
    // Bucket Entries
    // Chain Entries
    // =======
    // 
    
    const int numChains = int(_chains.size());
    
    _data.push_back(_numBuckets & 0xFF);
    _data.push_back((_numBuckets >> 8) & 0xFF);
    _data.push_back((_numBuckets >> 16) & 0xFF);
    _data.push_back((_numBuckets >> 24) & 0xFF);

    _data.push_back(numChains & 0xFF);
    _data.push_back((numChains >> 8) & 0xFF);
    _data.push_back((numChains >> 16) & 0xFF);
    _data.push_back((numChains >> 24) & 0xFF);

    for (int i = 0; i < _numBuckets; i++)
    {
        const int value = _buckets[i];
        _data.push_back(value & 0xFF);
        _data.push_back((value >> 8) & 0xFF);
        _data.push_back((value >> 16) & 0xFF);
        _data.push_back((value >> 24) & 0xFF);
    }

    for (int i = 0; i < numChains; i++)
    {
        const int value = _chains[i];
        _data.push_back(value & 0xFF);
        _data.push_back((value >> 8) & 0xFF);
        _data.push_back((value >> 16) & 0xFF);
        _data.push_back((value >> 24) & 0xFF);
    }
}


//
// poELF_RelocationTable
//

void poELF_RelocationTable:: add(const int relocationPos, const std::string& symbolName)
{
    auto& entry = _entries.emplace_back();
    entry.name = symbolName;
    entry.relocationPos = relocationPos;
}

void poELF_RelocationTable:: build(const int offset, poELF_HashTable& hash)
{
    const int jumpSlot = 7;
    for (auto& entry : _entries)
    {
        const int id = hash.lookup(entry.name);        
        if (id == -1) {
            std::cout << "Unable to find symbol id for " << entry.name << std::endl;
            continue;
        }
        entry.symbolId = id;

        Elf64_Rela rela = {};
        rela.r_offset = entry.relocationPos + offset;
        rela.r_info = (int64_t(entry.symbolId)<<32) | (0xffffffffL&jumpSlot);

        unsigned char* data = reinterpret_cast<unsigned char*>(&rela);
        for (size_t i = 0; i < sizeof(rela); i++) {
            _data.push_back(data[i]);
        }
    }
}

//
// poELF
//

poELF::poELF()
    :
    _entryPoint(0),
    _strtable(-1),
    _shstrtable(-1),
    _hashtable(-1),
    _textSection(-1),
    _readOnlyDataSection(-1),
    _initializedDataSection(-1),
    _dynamicSection(-1),
    _symtable(-1),
    _reltable(-1),
    _relatable(-1),
    _interp(-1),
    _plt(-1),
    _got_plt(-1)
{
}

void poELF::addPHDRSegment(std::vector<ElfProgramHeader64>& headers)
{
    ElfProgramHeader64& header = headers.emplace_back();
    std::memset(&header, 0, sizeof(header));

    header.p_type = int(poELF_SegmentType::PT_PHDR);
    header.p_flags = int(poELF_SegmentFlags::PF_R);
    header.p_offset = 0;
    header.p_vaddr = 0;
    header.p_paddr = 0;
    header.p_filesz = 0;
    header.p_memsz = 0;
    header.p_align = 16; // p_vaddr = p_offset % p_align

    ElfProgramHeader64& header2 = headers.emplace_back();
    std::memset(&header2, 0, sizeof(header2));

    header2.p_type = int(poELF_SegmentType::PT_LOAD);
    header2.p_flags = int(poELF_SegmentFlags::PF_R);
    header2.p_offset = 0;
    header2.p_vaddr = 0;
    header2.p_paddr = 0;
    header2.p_filesz = 0;
    header2.p_memsz = 0;
    header2.p_align = 16; // p_vaddr = p_offset % p_align
}

void poELF::addInterpSegment(poELF_Section& section, std::vector<ElfProgramHeader64>& headers)
{
    ElfProgramHeader64& header = headers.emplace_back();
    std::memset(&header, 0, sizeof(header));

    header.p_type = int(poELF_SegmentType::PT_INTERP);
    header.p_flags = int(poELF_SegmentFlags::PF_R);
    header.p_offset = 0; // gets set later
    header.p_vaddr = section.virtualAddress();
    header.p_paddr = section.virtualAddress();
    header.p_filesz = int(section.data().size());
    header.p_memsz = int(section.data().size());
    header.p_align = 16; // p_vaddr = p_offset % p_align
}

void poELF::addDynamicSegment(poELF_Section& section, std::vector<ElfProgramHeader64>& headers)
{
    ElfProgramHeader64& header = headers.emplace_back();
    std::memset(&header, 0, sizeof(header));

    header.p_type = int(poELF_SegmentType::PT_DYNAMIC);
    header.p_flags = int(poELF_SegmentFlags(int(poELF_SegmentFlags::PF_R) |
                int(poELF_SegmentFlags::PF_W)));
    header.p_offset = 0; // gets set later
    header.p_vaddr = section.virtualAddress();
    header.p_paddr = section.virtualAddress();
    header.p_filesz = int(section.data().size());
    header.p_memsz = int(section.data().size());
    header.p_align = 16; // p_vaddr = p_offset % p_align
}

void poELF::addProgramHeader(poELF_Section& section, std::vector<ElfProgramHeader64>& headers)
{
    ElfProgramHeader64& header = headers.emplace_back();
    std::memset(&header, 0, sizeof(header));

    poELF_SegmentFlags flags = (poELF_SegmentFlags)0;
    poELF_SegmentType type = poELF_SegmentType::PT_NULL;
    if ((int(section.flags()) & int(poELF_SectionFlags::SHF_EXECINSTR)) == 
        int(poELF_SectionFlags::SHF_EXECINSTR))
    {
        flags = poELF_SegmentFlags(
                int(flags) | int(poELF_SegmentFlags::PF_X)
                );
    }

    if (section.type() == poELF_SectionType::SHT_PROGBITS)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags(
                int(flags) | int(poELF_SegmentFlags::PF_R)
                );
        if ((int(section.flags()) & int(poELF_SectionFlags::SHF_WRITE)) ==
            int(poELF_SectionFlags::SHF_WRITE)) {
            flags = poELF_SegmentFlags( int(flags) | int(poELF_SegmentFlags::PF_W) );
        }
    }
    else if (section.type() == poELF_SectionType::SHT_NOBITS)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags(
                int(flags) | int(poELF_SegmentFlags::PF_R)
                );
    }
    else if (section.type() == poELF_SectionType::SHT_DYNAMIC)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags(int(poELF_SegmentFlags::PF_R) |
                int(poELF_SegmentFlags::PF_W));
    }
    else if (section.type() == poELF_SectionType::SHT_SYMTAB)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags::PF_R;
    }
    else if (section.type() == poELF_SectionType::SHT_HASH)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags::PF_R;
    }
    else if (section.type() == poELF_SectionType::SHT_STRTAB)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags::PF_R;
    }
    else if (section.type() == poELF_SectionType::SHT_RELA)
    {
        type = poELF_SegmentType::PT_LOAD;
        flags = poELF_SegmentFlags::PF_R;
    }


    header.p_type = int(type);
    header.p_flags = int(flags);
    header.p_offset = 0; // offset gets set later
    header.p_vaddr = section.virtualAddress();
    header.p_paddr = section.virtualAddress();
    header.p_filesz = int(section.data().size());
    header.p_memsz = int(section.data().size());
    header.p_align = 16; // p_vaddr = p_offset % p_align
}

void poELF::write(const std::string& filename)
{
    std::ofstream stream(filename, std::ios::binary);
    if (stream.is_open())
    {
        std::vector<ElfProgramHeader64> programHeaders;
        std::vector<ElfSectionHeader64> sectionHeaders;

        addPHDRSegment(programHeaders);

        // We create a header for each program section
        addInterpSegment(_sections[_interp], programHeaders);

        for (poELF_Section& section : _sections)
        {
            if (!section.needsSegment()) { continue; }

            addProgramHeader(section, programHeaders);
        }

        addDynamicSegment(_sections[_dynamicSection], programHeaders);

        const int pageSize = 4096;

        int dynamicHeaderId = -1;
        int interpHeaderId = -1;
        int segmentId = 3; // starts after the interp+phdr+phdr2 segments
        int sectionHeaderOffset = pageSize + sizeof(ElfProgramHeader64) * int(programHeaders.size());
        for (poELF_Section& section : _sections)
        {
            const int sectionMod = sectionHeaderOffset % pageSize; 
            if (sectionMod > 0)
            {
                sectionHeaderOffset += pageSize - sectionMod;
            }

            ElfSectionHeader64& header = sectionHeaders.emplace_back();
            header.sh_name = section.namePos();
            header.sh_type = int(section.type());
            header.sh_flags = int(section.flags());
            header.sh_addr = 0;
            header.sh_offset = sectionHeaderOffset;
            header.sh_size = int(section.data().size());
            header.sh_info = section.info();
            header.sh_addralign = 16;
            header.sh_entsize = section.entsize();
            header.sh_link = section.link();

            if (section.needsSegment()) {
                header.sh_addr = section.virtualAddress();

                assert(header.sh_addr % header.sh_addralign == 0);

                programHeaders[segmentId].p_offset = sectionHeaderOffset;

                assert(programHeaders[segmentId].p_offset % pageSize == 0);
                assert(programHeaders[segmentId].p_vaddr % pageSize == 0);
                assert(programHeaders[segmentId].p_vaddr % programHeaders[segmentId].p_align ==
                       programHeaders[segmentId].p_offset % programHeaders[segmentId].p_align);

                if (section.type() == poELF_SectionType::SHT_DYNAMIC) {
                    dynamicHeaderId = segmentId;
                } else if (section.virtualAddress() == _sections[_interp].virtualAddress()) {
                    interpHeaderId = segmentId;
                }

                segmentId++;
            }
            sectionHeaderOffset += int(section.data().size());
        }

        programHeaders[int(programHeaders.size()) - 1].p_offset = 
            programHeaders[dynamicHeaderId].p_offset;
        programHeaders[2].p_offset = 
            programHeaders[interpHeaderId].p_offset;

        // Setup the PHDR segments

        int virtualAddress = int(programHeaders[int(programHeaders.size()) - 1].p_vaddr + programHeaders[int(programHeaders.size())-1].p_memsz);
        const int sectionMod = virtualAddress % pageSize;
        if (sectionMod > 0)
        {
            virtualAddress += pageSize - sectionMod;
        }

        programHeaders[1].p_offset = pageSize;
        programHeaders[1].p_filesz = sizeof(ElfProgramHeader64) * int(programHeaders.size());
        programHeaders[1].p_vaddr = virtualAddress;
        programHeaders[1].p_memsz = programHeaders[1].p_filesz;
        programHeaders[1].p_paddr = virtualAddress;
        programHeaders[0].p_offset = pageSize;
        programHeaders[0].p_filesz = sizeof(ElfProgramHeader64) * int(programHeaders.size());
        programHeaders[0].p_memsz = programHeaders[1].p_memsz;
        programHeaders[0].p_vaddr = programHeaders[1].p_vaddr;
 
        assert(programHeaders[1].p_offset % pageSize == 0);
        assert(programHeaders[1].p_vaddr % pageSize == 0);
        assert(programHeaders[1].p_vaddr % programHeaders[segmentId].p_align ==
            programHeaders[1].p_offset % programHeaders[segmentId].p_align);
      
        // Begin writing the ELF header
       
        ElfHeader64 header = {};
        header.e_ident[0] = 0x7F;
        header.e_ident[1] = 'E';
        header.e_ident[2] = 'L';
        header.e_ident[3] = 'F';
        header.e_ident[4] = 2; // 64 bit
        header.e_ident[5] = 1; // little endian
        header.e_ident[6] = 1; // version 1
        header.e_type = 2; // 2 executable file
        header.e_machine = 0x3E; // x86-64
        header.e_version = 1;
        header.e_entry = _sections[_textSection].virtualAddress() + _entryPoint; // Entry point address
        header.e_phoff = pageSize;//sizeof(ElfHeader64); // Program header table offset
        header.e_shoff = sectionHeaderOffset; // Section header table offset
        header.e_flags = 0;
        header.e_ehsize = sizeof(ElfHeader64);
        header.e_phentsize = sizeof(ElfProgramHeader64); // Size of program header entry
        header.e_phnum = int(programHeaders.size()); // Number of program header entries
        header.e_shentsize = sizeof(ElfSectionHeader64); // Size of section header entry
        header.e_shnum = int(sectionHeaders.size()); // Number of section header entries
        header.e_shstrndx = _shstrtable; // Section header string table index
        stream.write((char*)&header, sizeof(header));

        int streamPos = sizeof(header);

        // Write initial padding.
        for (int i = 0; i < pageSize - streamPos; i++)
        {
            const char pad = 0;
            stream.write((char*)&pad, sizeof(pad));
        } 
        streamPos = pageSize;
         
        // Write the program headers.
        for (ElfProgramHeader64& header : programHeaders)
        {
            streamPos += sizeof(header);
            stream.write((char*)&header, sizeof(header));
        }        

        // Write the sections (with the segments).
        for (int i = 0; i < int(_sections.size()); i++)
        {
            ElfSectionHeader64& header = sectionHeaders[i];
            poELF_Section& section = _sections[i];
            
            const int padding = int(header.sh_offset) - streamPos;
            for (int i = 0; i < padding; i++)
            {
                const char pad = 0;
                stream.write((char*)&pad, sizeof(pad));
            }

            stream.write((char*)section.data().data(), int(section.data().size()));
            streamPos += padding + int(section.data().size());
        }

        // Write the section headers at the end.
        for (ElfSectionHeader64& header : sectionHeaders)
        {
            stream.write((char*)&header, sizeof(header));
        }
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
 
            stream.seekg(header.e_shoff);

            int dynSymPos = -1;
            int dynSymSize = 0;
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
                    dynSymPos = int(section.sh_offset);
                    dynSymSize = int(section.sh_size);
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

                    auto& symbol = _symbols.emplace_back(poELF_Symbol(
                                int(_symbols.size()),
                                int(sym.st_size),
                                int(sym.st_name)));
                    symbol.setType(sym.st_info);
                    symbol.setAddr(int(sym.st_value));
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
    std::cout << "ID | Name | Type | Addr" << std::endl;
    for (poELF_Symbol& symbol : _symbols)
    {
        std::cout << symbol.id() << " | " << symbol.getName() << " | " << symbol.type() << " | " << symbol.addr() << std::endl;
    }
}

void poELF::buildDynamicSection()
{
    poELF_Section& dynamic = _sections[_dynamicSection];
    std::vector<unsigned char>& data = dynamic.data();
    std::vector<Elf64_Dyn> entries;

    // DT_NEEDED library items
    for (int i = 0; i < int(_libraries.size()); i++)
    {
        Elf64_Dyn& lib = entries.emplace_back();
        lib.d_tag = int(poELF_DynamicTag::DT_NEEDED);
        lib.d_val = _libraries[i];
    }

    // DT_HASH
    Elf64_Dyn& hashtab = entries.emplace_back();
    hashtab.d_tag = int(poELF_DynamicTag::DT_HASH);
    hashtab.d_ptr = _sections[_hashtable].virtualAddress();

    // DT_STRTAB
    Elf64_Dyn& strtab = entries.emplace_back();
    strtab.d_tag = int(poELF_DynamicTag::DT_STRTAB);
    strtab.d_ptr = _sections[_strtable].virtualAddress();
    
    // DT_SYMTAB
    Elf64_Dyn& symtab = entries.emplace_back();
    symtab.d_tag = int(poELF_DynamicTag::DT_SYMTAB);
    symtab.d_ptr = _sections[_symtable].virtualAddress();

    if (_relatable != -1) {
        // DT_RELA
        Elf64_Dyn& relatab = entries.emplace_back();
        relatab.d_tag = int(poELF_DynamicTag::DT_RELA);
        relatab.d_ptr = _sections[_relatable].virtualAddress();

        // DT_RELASZ
        Elf64_Dyn& relasz = entries.emplace_back();
        relasz.d_tag = int(poELF_DynamicTag::DT_RELASZ);
        relasz.d_val = int(_sections[_relatable].data().size());

        // DT_RELAENT
        Elf64_Dyn& relaent = entries.emplace_back();
        relaent.d_tag = int(poELF_DynamicTag::DT_RELAENT);
        relaent.d_val = sizeof(Elf64_Rela);
    }

    // DT_STRSZ
    Elf64_Dyn& strsz = entries.emplace_back();
    strsz.d_tag = int(poELF_DynamicTag::DT_STRSZ);
    strsz.d_val = _sections[_strtable].data().size();

    // DT_SYMENT
    Elf64_Dyn& syment = entries.emplace_back();
    syment.d_tag = int(poELF_DynamicTag::DT_SYMENT);
    syment.d_val = sizeof(Elf64_Sym);

    if (_reltable != -1) {
        // DT_REL
        Elf64_Dyn& reltab = entries.emplace_back();
        reltab.d_tag = int(poELF_DynamicTag::DT_REL);
        reltab.d_ptr = _sections[_reltable].virtualAddress();

        // DT_RELSZ
        Elf64_Dyn& relsz = entries.emplace_back();
        relsz.d_tag = int(poELF_DynamicTag::DT_RELSZ);
        relsz.d_val = int(_sections[_reltable].data().size());

        // DT_RELENT
        Elf64_Dyn& relent = entries.emplace_back();
        relent.d_tag = int(poELF_DynamicTag::DT_RELENT);
        relent.d_val = sizeof(Elf64_Rel);
    }

    // DT_PLTREL
    Elf64_Dyn& pltrel = entries.emplace_back();
    pltrel.d_tag = int(poELF_DynamicTag::DT_PLTREL);
    pltrel.d_val = int(poELF_DynamicTag::DT_RELA);

    // DT_PLTRELSZ
    Elf64_Dyn& pltrelsz = entries.emplace_back();
    pltrelsz.d_tag = int(poELF_DynamicTag::DT_PLTRELSIZE);
    pltrelsz.d_val = int(_sections[_plt_rela].data().size());

    // DT_JUMPREL
    Elf64_Dyn& jmprel = entries.emplace_back();
    jmprel.d_tag = int(poELF_DynamicTag::DT_JMPREL);
    jmprel.d_val = _sections[_plt_rela].virtualAddress();

    if (_got_plt != -1) {
        // DT_GOTPLT
        Elf64_Dyn& got = entries.emplace_back();
        got.d_tag = int(poELF_DynamicTag::DT_PLTGOT);
        got.d_val = _sections[_got_plt].virtualAddress();
    }

    // DT_FLAGS
    Elf64_Dyn& flags = entries.emplace_back();
    flags.d_tag = int(poELF_DynamicTag::DT_FLAGS);
    flags.d_val = 0x8; // DF_BIND_NOW

    // DT_NULL
    Elf64_Dyn& nullEnt = entries.emplace_back();
    nullEnt.d_tag = int(poELF_DynamicTag::DT_NULL);
    
    // Write the entries
    for (const Elf64_Dyn& entry : entries) {
        for (size_t i = 0; i < sizeof(Elf64_Dyn); i++) {
            data.push_back(((unsigned char*)(&entry))[i]);
        }
    }
}

void poELF::initializeSections()
{
    buildHashTable();
    buildSymbolTable();
    buildStringTable();

    // GOT PLT
    poELF_Section& got = _sections[_got_plt];
    got.data().resize(got.size());

    std::vector<unsigned char>& interpData = _sections[_interp].data();

    // TODO: replace hardcoded version
    std::string interpName = "/lib64/ld-linux-x86-64.so.2";
    interpData.resize(int(interpName.size()) + 1);
    std::memcpy(interpData.data(), interpName.c_str(), int(interpName.size()));

    // Once we know the sizes of the sections we can calculate
    // the virtual address for them.

    const int pageSize = 4096;
    int virtualAddress = 0x400000;
    for (poELF_Section& section : _sections)
    {
        if (!section.needsSegment()) { continue; }
        section.setVirtualAddress(virtualAddress);
        if (section.type() == poELF_SectionType::SHT_DYNAMIC)
        {
            buildDynamicSection();
        }
        else if (section.type() == poELF_SectionType::SHT_RELA)
        {
            // Check this the PLT relocation data
            if (&_sections[_plt_rela] == &section) {
                // We need to generate this table after we have calculated the virtual address
                // of the PLT section, but before we calculate our virtual address.
                buildRelocationTable();
            }
        }

        virtualAddress += int(section.data().size());
        const int sectionMod = virtualAddress % pageSize;
        if (sectionMod > 0)
        {
            virtualAddress += pageSize - sectionMod;
        }
    }

    // We need to generate the string table with the names of the sections
    
    std::vector<unsigned char> buffer;

    // Write a null entry for the 0th entry 
    int pos = 1;
    buffer.push_back(0);
    for (poELF_Section& section : _sections)
    {
        section.setNamePos(pos);
        for (char c : section.name())
        {
            pos++;
            buffer.push_back(c);
        }
        pos++;
        buffer.push_back(0);
    }
 
    poELF_Section& strTable = _sections[_shstrtable];
    strTable.data().resize(1024);
    std::memcpy(strTable.data().data(), buffer.data(), buffer.size());

    // Set up section links
    _sections[_dynamicSection].setLink(_strtable);
    _sections[_hashtable].setLink(_symtable);
    _sections[_symtable].setLink(_strtable);
    _sections[_plt_rela].setLink(_symtable);

    // Set up section info
    if (_reltable != -1) { _sections[_reltable].setInfo(_textSection); }
    if (_relatable != -1) { _sections[_relatable].setInfo(_textSection); }
    _sections[_symtable].setInfo(1); // This is the last local symbol (we hard code it for now)
    _sections[_plt_rela].setInfo(_got_plt);
}

void poELF::buildRelocationTable()
{
    const int offset = _sections[_got_plt].virtualAddress();
    _pltTable.build(offset, _hashTable);

    poELF_Section& rel = _sections[_plt_rela];

    std::vector<unsigned char>& data = rel.data();
    data.resize(_pltTable.data().size());
    std::memcpy(data.data(), _pltTable.data().data(), data.size());
}

void poELF::buildHashTable()
{
    _hashTable.compute();
    //_hashTable.dump();

    poELF_Section& hash = _sections[_hashtable];
    
    std::vector<unsigned char>& data = hash.data();
    data.resize(_hashTable.data().size());
    std::memcpy(data.data(), _hashTable.data().data(), hash.size());
}

void poELF::buildSymbolTable()
{
    std::vector<Elf64_Sym> symbols;

    symbols.emplace_back(); // Add the SHN_UNDEF entry (all zeroes)

    for (int i = 1; i < int(_hashTable.symbols().size()); i++) {
        const int idx = _hashTable.symbols()[i] - 1;
        Elf64_Sym& sym = symbols.emplace_back();
 
        assert (idx >= 0);
        
        auto& symbol = _symbols[idx];
        sym.st_name = symbol.strPos();
        sym.st_info = 2 | (1 << 4); // STT_FUNC (2) + STB_GLOBAL (1)
        //sym.st_
        
    }

    std::vector<unsigned char>& data = _sections[_symtable].data();

    for (const Elf64_Sym& entry : symbols) {
        for (size_t i = 0; i < sizeof(Elf64_Sym); i++) {
            data.push_back(((unsigned char*)(&entry))[i]);
        }
    }
}

void poELF::addPltRelocation(const int relocationPos, const std::string& symbol)
{
    _pltTable.add(relocationPos, symbol);
}

void poELF::buildStringTable()
{
    // We need to generate the string table with names of libraries etc

    const std::vector<unsigned char>& buffer = _stringTable.data();

    poELF_Section& strTable = _sections[_strtable];
    strTable.data().resize(1024);
    std::memcpy(strTable.data().data(), buffer.data(), buffer.size());
}

void poELF::addSymbol(const std::string& name, const int id)
{
    poELF_Symbol symbol(id, 0, int(_stringTable.data().size()));
    _stringTable.write(name);
    symbol.setName(name);
    _symbols.push_back(symbol);

    _hashTable.addEntry(name);
}

void poELF::addLibrary(const std::string& name)
{
    _libraries.push_back(int(_stringTable.data().size())); // the first entry will be NULL
    _stringTable.write(name);
}

void poELF::add(const poELF_SectionType section, const std::string& name, const int size)
{
    int entsize = 0;
    bool needsSegment = false;
    poELF_SectionFlags flags = poELF_SectionFlags(0);
    if (name == ".text")
    {
        _textSection = int(_sections.size());
        flags = poELF_SectionFlags(int(poELF_SectionFlags::SHF_ALLOC) | int(poELF_SectionFlags::SHF_EXECINSTR));
        needsSegment = true;
    }
    else if (name == ".rodata")
    {
        _readOnlyDataSection = int(_sections.size());
        flags = poELF_SectionFlags::SHF_ALLOC;
        needsSegment = true;
    }
    else if (name == ".data")
    {
        _initializedDataSection = int(_sections.size());
        flags = poELF_SectionFlags(int(poELF_SectionFlags::SHF_ALLOC) | int(poELF_SectionFlags::SHF_WRITE));
        needsSegment = true;
    }
    else if (name == ".shstrtab")
    {
        // No attributes are need.
        _shstrtable = int(_sections.size());
    }
    else if (name == ".dynstr")
    {
       _strtable = int(_sections.size());
       flags = poELF_SectionFlags::SHF_ALLOC;
       needsSegment = true;
    }
    else if (name == ".dynamic")
    {
        _dynamicSection = int(_sections.size());
        flags = poELF_SectionFlags(int(poELF_SectionFlags::SHF_ALLOC) | int(poELF_SectionFlags::SHF_WRITE));
        needsSegment = true;
    }
    else if (name == ".hash")
    {
        _hashtable = int(_sections.size());
        flags = poELF_SectionFlags::SHF_ALLOC;
        needsSegment = true;
    }
    else if (name == ".symtab")
    {
        _symtable = int(_sections.size());
        flags = poELF_SectionFlags::SHF_ALLOC;
        needsSegment = true;
        entsize = sizeof(Elf64_Sym);
    }
    else if (name == ".rel.text")
    {
        _reltable = int(_sections.size());
        flags = poELF_SectionFlags::SHF_ALLOC;
        entsize = 0x10;
    }
    else if (name == ".rela.text")
    {
        _relatable = int(_sections.size());
        flags = poELF_SectionFlags::SHF_ALLOC;
        entsize = 0x18;
    }
    else if (name == ".interp")
    {
        _interp = int(_sections.size());
        flags = poELF_SectionFlags::SHF_ALLOC;
        needsSegment = true;
    }
    else if (name == ".tbss" || name == ".tdata")
    {
        // TLS (Thread Local Storage)

        flags = poELF_SectionFlags(int(poELF_SectionFlags::SHF_ALLOC) |
            int(poELF_SectionFlags::SHF_WRITE) |
            int(poELF_SectionFlags::SHF_TLS));
        needsSegment = true;
    }
    else if (name == ".plt")
    {
        // Procedure Linkage Table

        _plt = int(_sections.size());
        flags = poELF_SectionFlags( int(poELF_SectionFlags::SHF_ALLOC) |
            int(poELF_SectionFlags::SHF_EXECINSTR) );
        needsSegment = true;
        entsize = 0x10;
    }
    else if (name == ".got.plt")
    {
        // Global Offset Table for Procedure Link Table

        _got_plt = int(_sections.size());
        flags = poELF_SectionFlags( int(poELF_SectionFlags::SHF_ALLOC) |
            int(poELF_SectionFlags::SHF_WRITE) );
        entsize = 8;
        needsSegment = true;
    }
    else if (name == ".rela.plt")
    {
        _plt_rela = int(_sections.size());
        flags = poELF_SectionFlags( int(poELF_SectionFlags::SHF_ALLOC) |
                int(poELF_SectionFlags::SHF_INFO));
        needsSegment = true;
        entsize = 0x18;
    }

    _sections.emplace_back(poELF_Section(section, flags, name, size, entsize, needsSegment));
}


