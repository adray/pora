#include "poCOFF.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <string.h>

using namespace po;

struct FileHeader {
    char Name[16];
    char Date[12];
    char UserId[6];
    char GroupId[6];
    char Mode[8];
    char BodyLength[10];
    char EndOfHeader[2];
};

struct SymbolDescriptor
{
    uint16_t Signature[2];
    uint16_t Version;
    uint16_t Machine;
    uint32_t Timestamp;
    int32_t Length;
    union
    {
        uint16_t Hint;
        uint16_t Ordinal;
        uint16_t Value;
    };
    uint16_t Type;
};


// 1 byte aligned
struct PeHeader {
    uint32_t mMagic; // PE\0\0 or 0x00004550
    uint16_t mMachine;
    uint16_t mNumberOfSections;
    uint32_t mTimeDateStamp;
    uint32_t mPointerToSymbolTable;
    uint32_t mNumberOfSymbols;
    uint16_t mSizeOfOptionalHeader;
    uint16_t mCharacteristics;
};

//==========================
// poArchiveLinkMember1
//==========================

void poArchiveLinkMember1::addSymbolOffset(int offset)
{
    _offsets.push_back(offset);
}

void poArchiveLinkMember1::addSymbolName(const std::string& name)
{
    _names.push_back(name);
}

//===========================
// poArchiveLinkMember2
//===========================

void poArchiveLinkMember2::addSymbolOffset(int offset)
{
    _offsets.push_back(offset);
}

void poArchiveLinkMember2::addSymbolIndex(int index)
{
    _indices.push_back(index);
}

void poArchiveLinkMember2::addSymbolName(const std::string& name)
{
    _names.push_back(name);
}

//=============================
// poCommonObjectFileFormat
//=============================

bool poCommonObjectFileFormat::open(const std::string& filename)
{
    std::ifstream ss(filename, std::ios::binary);
    if (!ss.good())
    {
        return false;
    }

    char signature[9] = {};
    ss.read((char*)signature, sizeof(signature)-1);

    if (strcmp(signature, "!<arch>\n") != 0)
    {
        return false;
    }

    FileHeader header;
    ss.read((char*)&header, sizeof(header));

    if (header.Name[0] != '/')
    {
        return false;
    }
    parseFirstLinkerHeader(ss);

    ss.read((char*)&header, sizeof(header));

    if (header.Name[0] != '/')
    {
        return false;
    }
    parseSecondLinkerHeader(ss);

    // Long name section?
    ss.read((char*)&header, sizeof(header));
    if (header.Name[0] == '/' && header.Name[1] == '/')
    {
        int consumed = 0;
        const int length = strtol(header.BodyLength, nullptr, 10);
        while (consumed < length)
        {
            std::string name;
            while (ss.peek() != 0)
            {
                name += (char)ss.get();
            }
            ss.get();

            consumed += int(name.length()) + 1;
        }
    }

    ///std::cout << "Linker headers done" << std::endl;

    for (int i = 0; i < int(_linkerMember2.numSymbols()); i++)
    {
        const auto& name = _linkerMember2.name(i);
        const int index = _linkerMember2.indicies(i);
        const int offset = _linkerMember2.offset(index-1);

        SymbolDescriptor sd;
        ss.seekg(offset);
        ss.read((char*)&header, sizeof(header));
        ss.read((char*)&sd, sizeof(sd));

        std::string importName;
        while (ss.peek() != 0)
        {
            importName += (char)ss.get();
        }
        ss.get();

        std::string dllName;
        while (ss.peek() != 0)
        {
            dllName += (char)ss.get();
        }
        ss.get();

        const int importType = int(sd.Type & 0x3);
        const int importNameType = int((sd.Type >> 2) & 0x7);

        _imports.emplace_back(importType, importNameType, sd.Ordinal, importName, dllName, sd.Machine);
    }

    return true;
}

void po::poCommonObjectFileFormat::parseSecondLinkerHeader(std::ifstream& ss)
{
    //std::cout << "2nd Linker member" << std::endl;

    uint32_t numMembers;
    ss.read((char*)&numMembers, sizeof(numMembers));
    //std::cout << "Num members: " << numMembers << std::endl;
    for (uint32_t i = 0; i < numMembers; i++)
    {
        uint32_t offsets;
        ss.read((char*)&offsets, sizeof(offsets));
        _linkerMember2.addSymbolOffset(offsets);
    }

    uint32_t numSymbols;
    ss.read((char*)&numSymbols, sizeof(numSymbols));
    //std::cout << "Num symbols: " << numSymbols << std::endl;
    for (uint32_t i = 0; i < numSymbols; i++)
    {
        uint16_t indicies;
        ss.read((char*)&indicies, sizeof(indicies));
        _linkerMember2.addSymbolIndex(indicies);
    }

    for (uint32_t i = 0; i < numSymbols; i++)
    {
        std::string name;
        while (ss.peek() != 0)
        {
            name += (char)ss.get();
        }
        ss.get();
        _linkerMember2.addSymbolName(name);
    }
}

void po::poCommonObjectFileFormat::parseFirstLinkerHeader(std::ifstream& ss)
{
    //std::cout << "1st Linker member" << std::endl;

    unsigned char numSymbols[4];
    ss.read((char*)numSymbols, sizeof(numSymbols));

    uint32_t num = uint32_t(numSymbols[0]) << 24 | uint32_t(numSymbols[1]) << 16 | uint32_t(numSymbols[2]) << 8 | uint32_t(numSymbols[3]);

    //std::cout << "Num symbols: " << num << std::endl;

    for (uint32_t i = 0; i < num; i++)
    {
        unsigned char offset[4];
        ss.read((char*)offset, sizeof(offset));

        uint32_t off = uint32_t(offset[0]) << 24 | uint32_t(offset[1]) << 16 | uint32_t(offset[2]) << 8 | uint32_t(offset[3]);
        _linkerMember1.addSymbolOffset(off);
    }
    for (uint32_t i = 0; i < num; i++)
    {
        std::string name;
        while (ss.peek() != 0)
        {
            name += (char)ss.get();
        }
        ss.get();
        _linkerMember1.addSymbolName(name);
    }
}

void poCommonObjectFileFormat::dump() const
{
    for (const poImport& import : _imports)
    {
        std::string importTypeString;
        switch (import.importType())
        {
        case 0:
            importTypeString = "Executable code.";
            break;
        case 1:
            importTypeString = "Data.";
            break;
        case 2:
            importTypeString = "CONST";
            break;
        }

        std::string importNameTypeString;
        switch (import.importNameType())
        {
        case 0:
            importNameTypeString = "IMPORT_OBJECT_ORDINAL";
            break;
        case 1:
            importNameTypeString = "IMPORT_OBJECT_NAME";
            break;
        case 2:
            importNameTypeString = "IMPORT_OBJECT_NAME_NOPREFIX";
            break;
        case 3:
            importNameTypeString = "IMPORT_OBJECT_NAME_UNDECORATE";
            break;
        }

        std::cout << import.importName() << /*std::hex << "|" << offset <<*/ "|" << importTypeString << "|" << importNameTypeString << "|"
            << import.machine() << "|" << import.importOrdinal() << "|" << import.importName() << "|" << import.dllName() << std::endl;
    }
}
