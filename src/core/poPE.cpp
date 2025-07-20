#include "poPE.h"
#include <fstream>
#include <cstdint>
#include <iostream>
#include <string.h>

#include <assert.h>

using namespace po;

/*
* Concepts:
* File pointer: offset relative to the file stored on disk.
* Relative virtual address (RVA): this is the address of an item after it is loaded into memory
* with the base of the address subtracted from it.
* Virtual address (VA): Same as RVA, except the base address is not subtracted from it.
*/

namespace po
{

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

    // 1 byte aligned, 112 bytes
    struct Pe32PlusOptionalHeader {
        uint16_t mMagic; // 0x020b - PE32+ (64 bit)
        uint8_t  mMajorLinkerVersion;
        uint8_t  mMinorLinkerVersion;
        uint32_t mSizeOfCode;
        uint32_t mSizeOfInitializedData;
        uint32_t mSizeOfUninitializedData;
        uint32_t mAddressOfEntryPoint;
        uint32_t mBaseOfCode;
        uint64_t mImageBase;
        uint32_t mSectionAlignment;
        uint32_t mFileAlignment;
        uint16_t mMajorOperatingSystemVersion;
        uint16_t mMinorOperatingSystemVersion;
        uint16_t mMajorImageVersion;
        uint16_t mMinorImageVersion;
        uint16_t mMajorSubsystemVersion;
        uint16_t mMinorSubsystemVersion;
        uint32_t mWin32VersionValue;
        uint32_t mSizeOfImage;
        uint32_t mSizeOfHeaders;
        uint32_t mCheckSum;
        uint16_t mSubsystem;
        uint16_t mDllCharacteristics;
        uint64_t mSizeOfStackReserve;
        uint64_t mSizeOfStackCommit;
        uint64_t mSizeOfHeapReserve;
        uint64_t mSizeOfHeapCommit;
        uint32_t mLoaderFlags;
        uint32_t mNumberOfRvaAndSizes;
    };

    struct PeSectionTable {
        char mName[8];
        uint32_t mVirtualSize;
        uint32_t mVirtualAddress;
        uint32_t mSizeOfRawData;
        uint32_t mPointerToRawData;
        uint32_t mPointerToRelocations;
        uint32_t mPointerToLineNumbers;
        uint16_t mNumberOfRelocations;
        uint16_t mNumberOfLineNumbers;
        uint32_t mCharacteristics;
    };

    struct PeImportDirectory {
        uint32_t mCharacteristics; /*The RVA of the import lookup table*/
        uint32_t mTimestamp;
        uint32_t mForwarderChain;
        uint32_t mName; /* RVA of an ASCI string containing the name of the DLL */
        uint32_t mImportAddressTable; /*The RVA of the import address table.*/
    };

    struct PeExportDirectory {
        uint32_t mExportFlags;
        uint32_t mTimestamp;
        uint16_t mMajorVersion;
        uint16_t mMinorVersion;
        uint32_t mNameRVA;
        uint32_t mOrdinalBase;
        uint32_t mNumAddressTableEntries;
        uint32_t mNumNamePointers;
        uint32_t mExportAddressTableRVA;
        uint32_t mNamePointerRVA;
        uint32_t mOrdinalTableRVA;
    };

    struct PeImageDataDirectory {
        uint32_t mVirtualAddress;
        uint32_t mSize;
    };

    struct poSection {
        int _filePos; // aligned to the mFileAlignment
        int _imagePos; // aligned to the mSectionAlignment
        int _filePaddingSize;
        int _fileSize; 
        int _imageSize; 
    };

    struct poPEImage
    {
        uint8_t dos_magic_number[64];
        uint8_t standard_stub[128];
        PeHeader header;
        Pe32PlusOptionalHeader optionalHeader;
        PeImageDataDirectory dirs[16];
        std::vector<poSection> sectionData;
    };
}

//====================================
// Portable Executable Import Entry
//====================================

poImportEntry::poImportEntry(const std::string& name, int ordinal)
    :
    _addressRVA(0),
    _name(name),
    _nameRVA(0),
    _ordinal(ordinal)
{
}

// ====================================
// Portable Executable Import Table
// ====================================

poPortableExecutableImportTable::poPortableExecutableImportTable(const std::string& dllName)
    :
    _dllName(dllName),
    _importPosition(0)
{
}

void poPortableExecutableImportTable::addImport(const std::string& name, const int ordinal)
{
    _imports.emplace_back(name, ordinal);
}

//====================================
// Portable Executable Export Table
//====================================

void poPortableExecutableExportTable::addExport(int nameRVA, const char* name)
{
    auto& exports = _forwarderExports.emplace_back();
    exports._name = name;
    exports._nameRVA = nameRVA;
}

void poPortableExecutableExportTable::addExport(int ordinal, int addressRVA, const char* name)
{
    auto& exports = _exports.emplace_back();
    exports._ordinal = ordinal;
    exports._name = name;
    exports._addressRVA = addressRVA;
}

void poPortableExecutableExportTable::dump()
{
    for (auto& e : _exports)
    {
        std::cout << e._ordinal << "|0x"  << std::hex << e._addressRVA << "|" << std::dec << e._name << std::endl;
    }

    for (auto& e : _forwarderExports)
    {
        std::cout << e._name << std::endl;
    }
}

//=========================
// Portable Executable
//========================

poPortableExecutable::poPortableExecutable()
    :
    _textSection(-1),
    _initializedSection(-1),
    _uninitializedSection(-1),
    _iData(-1),
    _readonlySection(-1),
    _entryPoint(0),
    _peImage(nullptr)
{
}

void poPortableExecutable::addDataSections()
{
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::EXPORT, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::IMPORT, 0, 512);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::RESOURCE, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::EXCEPTION, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::CERTIFICATE, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::BASE_RELOCATION, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::DEBUG, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::ARCHITECTURE, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::GLOBAL_PTR, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::TLS, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::LOAD_CONFIG, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::BOUND_IMPORT, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::IAT, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::DELAY_IMPORT, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::CLR_RUNTIME, 0, 0);
    _dataSections.emplace_back(poSectionType::IDATA, poDataDirectoryType::RESERVED, 0, 0);
}

void poPortableExecutable::addSection(const poSectionType type, const int size)
{
    switch (type)
    {
    case poSectionType::TEXT:
        _textSection = int(_sections.size());
        break;
    case poSectionType::INITIALIZED:
        _initializedSection = int(_sections.size());
        break;
    case poSectionType::UNINITIALIZED:
        _uninitializedSection = int(_sections.size());
        break;
    case poSectionType::IDATA:
        _iData = int(_sections.size());
        break;
    }

    poPortableExecutableSection& section = _sections.emplace_back(type);
    section.data().resize(size);
}

poPortableExecutableImportTable& poPortableExecutable::addImportTable(const std::string& name)
{
    return _imports.emplace_back(name);
}

void poPortableExecutable::buildImportNameTable(std::vector<unsigned char>& importNameTable)
{
    int pos = 0;
    for (poPortableExecutableImportTable& table : _imports)
    {
        table.setImportPosition(pos);

        const std::string& name = table.dllName();
        importNameTable.resize(importNameTable.size() + int(name.size()) + 1);
        std::memcpy(importNameTable.data() + pos, name.c_str(), name.size());
        pos += int(name.size()) + 1;
    }
}

void poPortableExecutable::buildImportHintTable(std::vector<unsigned char>& importHintTable, const int imageBase)
{
    /* Calculate the hint table rva base */

    int rvaBase = imageBase;
    const int directorySize = sizeof(PeImportDirectory) * int(_imports.size() + 1);
    rvaBase += directorySize;
    for (poPortableExecutableImportTable& table : _imports)
    {
        const int importTableSize = int(table.imports().size() + 1) * 8; // 8 bytes per import entry (RVA + hint)
        rvaBase += importTableSize * 2; // Update the base for the next table
    }

    /* Generate the table entries */

    int pos = 0;
    for (poPortableExecutableImportTable& table : _imports)
    {
        for (poImportEntry& import : table.imports())
        {
            import.setNameRVA(pos + rvaBase);

            importHintTable.resize(importHintTable.size() + 2); // 2 bytes for the hint (which we leave blank)
            pos += 2;

            const std::string& name = import.name();
            importHintTable.resize(importHintTable.size() + name.size() + 1);
            std::memcpy(importHintTable.data() + pos, name.c_str(), name.size());
            pos += int(name.size()) + 1;

            if (pos % 2 != 0) // Align to 2 bytes
            {
                importHintTable.push_back(0);
                pos++;
            }
        }
    }
}

void poPortableExecutable::buildImportLookupTable(std::vector<unsigned char>& importLookupTable)
{
    for (poPortableExecutableImportTable& table : _imports)
    {
        for (poImportEntry& import : table.imports())
        {
            import.setAddressRVA(int(importLookupTable.size()));

            const uint64_t entry = (import.nameRVA() & 0xFFFFFFFF);

            importLookupTable.push_back(entry & 0xFF);
            importLookupTable.push_back((entry >> 8) & 0xFF);
            importLookupTable.push_back((entry >> 16) & 0xFF);
            importLookupTable.push_back((entry >> 24) & 0xFF);
            importLookupTable.push_back((entry >> 32) & 0xFF);
            importLookupTable.push_back((entry >> 40) & 0xFF);
            importLookupTable.push_back((entry >> 48) & 0xFF);
            importLookupTable.push_back((entry >> 56) & 0xFF);
        }

        for (int i = 0; i < 8; i++)
        {
            importLookupTable.push_back(0);
        }
    }
}

void poPortableExecutable::genImports(const int imagePos, poPortableExecutableSection& section, const poPortableExecutableData& data)
{
    /* Build Import Name Table */
    std::vector<unsigned char> importNameTable;
    buildImportNameTable(importNameTable);

    /* Build Hint/Name table */
    std::vector<unsigned char> importHintNameTable;
    buildImportHintTable(importHintNameTable, imagePos + data.offset());

    /* Build Import Table */
    std::vector<unsigned char> importTable;
    buildImportLookupTable(importTable);

    /* Create the tables */

    const int numDLLs = int(_imports.size());
    const int importLookupTablePos = data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1);
    const int importAddressTablePos = data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + int(importTable.size());
    const int importHintTablePos = data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + int(importTable.size()) * 2;
    const int importNameTablePos = data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + int(importTable.size()) * 2 + int(importHintNameTable.size());

    std::memcpy(section.data().data() + data.offset() + importLookupTablePos, importTable.data(), int(importTable.size()));
    std::memcpy(section.data().data() + data.offset() + importAddressTablePos, importTable.data(), int(importTable.size()));
    std::memcpy(section.data().data() + data.offset() + importHintTablePos, importHintNameTable.data(), int(importHintNameTable.size()));
    std::memcpy(section.data().data() + data.offset() + importNameTablePos, importNameTable.data(), int(importNameTable.size()));

    /* Import Directory entry per DLL */

    PeImportDirectory* imports = reinterpret_cast<PeImportDirectory*>(section.data().data() + data.offset());
    for (int i = 0; i < numDLLs; i++)
    {
        imports[i].mName = imagePos + importNameTablePos;
        imports[i].mCharacteristics = imagePos + importLookupTablePos;
        imports[i].mImportAddressTable = imagePos + importAddressTablePos;
    }
    /* last import is all NULLs */

    for (poPortableExecutableImportTable& table : _imports)
    {
        for (poImportEntry& import : table.imports())
        {
            import.setAddressRVA(import.addressRVA() + imagePos + importAddressTablePos);
        }
    }
}

constexpr int IMAGE_SCN_CNT_CODE = 0x00000020;
constexpr int IMAGE_SCN_MEM_EXECUTE = 0x20000000;
constexpr int IMAGE_SCN_MEM_READ = 0x40000000;
constexpr int IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080;
constexpr int IMAGE_SCN_MEM_WRITE = 0x80000000;
constexpr int IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040;

static int RoundToAlignment(int size, int alignment)
{
    const int remainder = size % alignment;
    if (remainder == 0) { return size; }
    return ((size / alignment) + 1) * alignment;
}

void poPortableExecutable::initializeSections()
{
    if (_textSection == -1 || _uninitializedSection == -1 || _initializedSection == -1 || _iData == -1)
    {
        return;
    }

    addDataSections();

    const poPortableExecutableSection& section = _sections[_textSection];
    const poPortableExecutableSection& uninitializedSection = _sections[_uninitializedSection];
    const poPortableExecutableSection& initializedSection = _sections[_initializedSection];

    _peImage = new poPEImage();

    const int numDataDirectories = 16;
    PeImageDataDirectory* dirs = _peImage->dirs;

    PeHeader& header = _peImage->header;
    header.mMagic = 0x50 | (0x45 << 8) | (0x0 << 16) | (0x0 << 24);
    header.mMachine = 0x64 | (0x86 << 8); /* x86_64 */
    header.mNumberOfSections = int16_t(_sections.size());
    header.mTimeDateStamp = 0; /* milliseconds since 1970 1st Jan */
    header.mCharacteristics = 0x22; /* 0x2 = IMAGE_FILE_EXECUTABLE_IMAGE, 0x20 = IMAGE_FILE_LARGE_ADDRESS_AWARE  */
    header.mSizeOfOptionalHeader = sizeof(Pe32PlusOptionalHeader) + sizeof(_peImage->dirs);

    const int sizeOfHeaders = sizeof(_peImage->standard_stub) +
        sizeof(_peImage->dos_magic_number) +
        sizeof(header) +
        sizeof(Pe32PlusOptionalHeader) +
        sizeof(_peImage->dirs) +
        sizeof(PeSectionTable) * int(_sections.size());

    Pe32PlusOptionalHeader& optionalHeader = _peImage->optionalHeader;
    optionalHeader.mMagic = 0x020b;
    optionalHeader.mSizeOfCode = uint32_t(section.data().size());
    optionalHeader.mSizeOfUninitializedData = _uninitializedSection > -1 ? uint32_t(uninitializedSection.data().size()) : 0;
    optionalHeader.mSizeOfInitializedData = _initializedSection > -1 ? uint32_t(initializedSection.data().size()) : 0;
    optionalHeader.mMajorOperatingSystemVersion = 0x6;
    optionalHeader.mMajorSubsystemVersion = 0x6;
    optionalHeader.mAddressOfEntryPoint = 0x01000 /*hardcoded to text section..*/ + _entryPoint;
    optionalHeader.mBaseOfCode = 0x01000 /*hardcoded to text section.. (with padding, can't be 0)*/;
    optionalHeader.mImageBase = 0x0000000140000000; //0x00400000;
    optionalHeader.mSectionAlignment = 0x1000; // standard section alignment
    optionalHeader.mFileAlignment = 0x200; // standard file alignment
    optionalHeader.mSizeOfHeaders = RoundToAlignment(sizeOfHeaders, optionalHeader.mFileAlignment); // size of headers and sections rounded to file alignment
    optionalHeader.mSizeOfStackReserve = 0x0100000;
    optionalHeader.mSizeOfHeapReserve = 0x0100000;
    optionalHeader.mSizeOfHeapCommit = 0x1000;
    optionalHeader.mSizeOfStackCommit = 0x1000;
    optionalHeader.mNumberOfRvaAndSizes = numDataDirectories;
    optionalHeader.mSubsystem = 0x3; /* GUI application=0x2, Console=0x3 */
    optionalHeader.mMajorLinkerVersion = 0x0E;
    optionalHeader.mMinorLinkerVersion = 0x2A;

    const int offset = 0x3C;
    int sectionFilePos = optionalHeader.mSizeOfHeaders;
    int imagePos = optionalHeader.mBaseOfCode;

    // Compute sections

    for (auto& section : _sections)
    {
        auto& data = _peImage->sectionData.emplace_back();
        data._filePos = sectionFilePos;
        data._fileSize = int(section.data().size());
        data._imagePos = imagePos;
        data._imageSize = int(section.data().size());

        sectionFilePos = RoundToAlignment(data._filePos + data._fileSize, optionalHeader.mFileAlignment);
        imagePos = RoundToAlignment(data._imagePos + data._fileSize, optionalHeader.mSectionAlignment);

        data._filePaddingSize = sectionFilePos - data._filePos - data._fileSize;
    }
    optionalHeader.mSizeOfImage = RoundToAlignment(imagePos, optionalHeader.mSectionAlignment);

    // Compute data directories

    for (int i = 0; i < numDataDirectories; i++)
    {
        auto& section = _dataSections[i];
        auto& dir = dirs[i];

        bool includeDirectory = true;
        switch (section.type())
        {
        case poDataDirectoryType::IMPORT:
            genImports(_peImage->sectionData[_iData]._imagePos, _sections[_iData], section);
            break;
        default:
            includeDirectory = false;
            break;
        }

        if (includeDirectory)
        {
            switch (section.parent())
            {
            case poSectionType::IDATA:
                dir.mVirtualAddress = _peImage->sectionData[_iData]._imagePos + section.offset();
                dir.mSize = section.size();
                break;
            }
        }
        else
        {
            dir.mSize = 0;
            dir.mVirtualAddress = 0;
        }
    }

    *reinterpret_cast<int32_t*>(_peImage->dos_magic_number) = 0x5A4D;
    *reinterpret_cast<int32_t*>(_peImage->dos_magic_number + offset) = sizeof(_peImage->dos_magic_number) + sizeof(_peImage->standard_stub);
}

void poPortableExecutable::write(const std::string& filename)
{
    if (_textSection == -1 || _uninitializedSection == -1 || _initializedSection == -1 || _iData == -1)
    {
        return;
    }

    std::ofstream stream(filename, std::ios::binary);
    if (stream.is_open())
    {
        // Write headers
        stream.write((char*)_peImage->dos_magic_number, sizeof(_peImage->dos_magic_number));
        stream.write((char*)_peImage->standard_stub, sizeof(_peImage->standard_stub));
        stream.write((char*)&_peImage->header, sizeof(_peImage->header));
        stream.write((char*)&_peImage->optionalHeader, sizeof(_peImage->optionalHeader));
        stream.write((char*)_peImage->dirs, sizeof(_peImage->dirs));

        const std::string textName = ".text";
        const std::string initName = ".data";
        const std::string uninitializedName = ".bss";
        const std::string iDataName = ".idata";

        // Write section headers
        for (size_t i = 0; i < _sections.size(); i++)
        {
            auto& section = _sections[i];
            auto& data = _peImage->sectionData[i];

            PeSectionTable table = {};
            switch (section.type())
            {
            case poSectionType::TEXT:
                std::memcpy(table.mName, textName.data(), textName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
                break;
            case poSectionType::INITIALIZED:
                std::memcpy(table.mName, initName.data(), initName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_INITIALIZED_DATA /* | IMAGE_SCN_MEM_WRITE*/ | IMAGE_SCN_MEM_READ;
                break;
            case poSectionType::UNINITIALIZED:
                std::memcpy(table.mName, uninitializedName.data(), uninitializedName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                break;
            case poSectionType::IDATA:
                std::memcpy(table.mName, iDataName.data(), iDataName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                break;
            }
            table.mVirtualSize = uint32_t(section.data().size());
            table.mVirtualAddress = data._imagePos;
            table.mSizeOfRawData = uint32_t(section.data().size());
            table.mPointerToRawData = data._filePos;

            stream.write((char*)&table, sizeof(table));
        }

        const int sizeOfHeaders = sizeof(_peImage->standard_stub) +
            sizeof(_peImage->dos_magic_number) +
            sizeof(_peImage->header) +
            sizeof(Pe32PlusOptionalHeader) +
            sizeof(_peImage->dirs) +
            sizeof(PeSectionTable) * int(_sections.size());
        assert(_peImage->optionalHeader.mSizeOfHeaders >= uint32_t(sizeOfHeaders));

        std::vector<char> sectionPadding(_peImage->optionalHeader.mSizeOfHeaders - sizeOfHeaders);
        stream.write((char*)sectionPadding.data(), sectionPadding.size()); // Write padding

        // Write sections
        for (size_t i = 0; i < _sections.size(); i++)
        {
            auto& section = _sections[i];
            auto& data = _peImage->sectionData[i];

            const int paddingSize = data._filePaddingSize;
            if (paddingSize > 0)
            {
                std::vector<char> padding(paddingSize);
                stream.write((char*)padding.data(), padding.size()); // Write padding
            }
            if (section.data().size() > 0)
            {
                stream.write((char*)section.data().data(), section.data().size());
            }
        }
    }
}


bool poPortableExecutable::open(const std::string& filename)
{
    std::ifstream stream(filename, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    uint8_t dos_magic_number[64] = {};

    stream.read((char*)dos_magic_number, sizeof(dos_magic_number));

    if (*reinterpret_cast<int16_t*>(dos_magic_number) != 0x5A4D)
    {
        return false;
    }
    
    const int offset = 0x3C;
    const int32_t headerPos = *reinterpret_cast<int32_t*>(dos_magic_number + offset);
    const int32_t standardStubSize = headerPos - sizeof(dos_magic_number);

    std::vector<char> standard_stub(standardStubSize);
    stream.read((char*)standard_stub.data(), standardStubSize);

    PeHeader header = {};
    stream.read((char*)&header, sizeof(header));

    if (header.mMachine != 0x8664)
    {
        return false;
    }

    Pe32PlusOptionalHeader optionalHeader = {};
    stream.read((char*)&optionalHeader, sizeof(optionalHeader));

    const int numDirectories = 16;
    if (optionalHeader.mNumberOfRvaAndSizes != numDirectories)
    {
        return false;
    }

    PeImageDataDirectory dir[numDirectories] = {};
    stream.read((char*)dir, sizeof(dir));

    std::vector<PeSectionTable> sectionHeaders(header.mNumberOfSections);
    stream.read((char*)sectionHeaders.data(), sizeof(PeSectionTable) * header.mNumberOfSections);

    std::vector<int> sectionsIds;

    for (size_t i = 0; i < sectionHeaders.size(); i++)
    {
        const auto& sectionHeader = sectionHeaders[i];
        std::string name = sectionHeader.mName;
        poSectionType type;
        const size_t id = _sections.size();
        if (name == ".idata")
        {
            type = poSectionType::IDATA;
            _iData = int(id);
        }
        else if (name == ".text")
        {
            type = poSectionType::TEXT;
            _textSection = int(id);
        }
        else if (name == ".rdata")
        {
            type = poSectionType::READONLY;
            _readonlySection = int(id);
        }
        else if (name == ".data")
        {
            type = poSectionType::INITIALIZED;
            _initializedSection = int(id);
        }
        else if (name == ".bss")
        {
            type = poSectionType::UNINITIALIZED;
            _uninitializedSection = int(id);
        }
        else
        {
            sectionsIds.push_back(-1);
            continue;
        }

        sectionsIds.push_back(int(id));

        const auto& section = _sections.emplace_back(type);
        auto& data = _sections.rbegin()->data();
        data.resize(sectionHeader.mSizeOfRawData);

        stream.seekg(std::streampos(sectionHeader.mPointerToRawData));
        stream.read((char*)data.data(), sectionHeader.mSizeOfRawData);

        for (int i = 0; i < numDirectories; i++)
        {
            auto& directory = dir[i];
            if (directory.mVirtualAddress >= sectionHeader.mVirtualAddress &&
                directory.mVirtualAddress + directory.mSize < sectionHeader.mVirtualAddress + sectionHeader.mVirtualSize)
            {
                _dataSections.emplace_back(type, (poDataDirectoryType)i, directory.mVirtualAddress - sectionHeader.mVirtualAddress, directory.mSize);
            }
        }
    }

    // Get exports

    int exportTable;
    int headerId;
    findExportTable(sectionsIds, &exportTable, &headerId);
    if (exportTable != -1)
    {
        getExports(int(exportTable), sectionHeaders[headerId].mVirtualAddress);
    }

    return true;
}

void poPortableExecutable::findExportTable(const std::vector<int>& sectionIds, int* exportIndex, int* headerId)
{
    *exportIndex = -1;
    for (size_t i = 0; i < _dataSections.size(); i++)
    {
        if (_dataSections[i].type() == poDataDirectoryType::EXPORT)
        {
            *exportIndex = int(i);
            break;
        }
    }

    if (*exportIndex != -1)
    {
        auto& exportDirectory = _dataSections[*exportIndex];
        for (int i = 0; i < sectionIds.size(); i++)
        {
            if (sectionIds[i] > -1 && _sections[sectionIds[i]].type() == exportDirectory.parent())
            {
                *headerId = (i);
                break;
            }
        }
    }
}

void poPortableExecutable::getExports(const int exportIndex, const int rva)
{
    if (exportIndex == -1) { return; }

    const auto& dir = _dataSections[exportIndex];
    if (dir.parent() != poSectionType::READONLY)
    {
        return;
    }

    const auto& section = _sections[_readonlySection];
    const unsigned char* data = section.data().data() + dir.offset();
    const PeExportDirectory* exports = reinterpret_cast<const PeExportDirectory*>(data);

    const uint32_t addressTableOffset = exports->mExportAddressTableRVA - rva;

    const uint32_t* addresses = reinterpret_cast<const uint32_t*>(section.data().data() + addressTableOffset);
    for (uint32_t i = 0; i < exports->mNumAddressTableEntries; i++)
    {
        // Add to exports if it is a forwarder RVA
        if (int(addresses[i]) >= rva && addresses[i] - rva < section.data().size())
        {
            const char* name = reinterpret_cast<const char*>(section.data().data() + addresses[i] - rva);
            _exports.addExport(addresses[i], name);
        }
    }

    const uint16_t* exportOrdinals = reinterpret_cast<const uint16_t*>(section.data().data() + exports->mOrdinalTableRVA - rva);

    const uint32_t* exportNames = reinterpret_cast<const uint32_t*>(section.data().data() + exports->mNamePointerRVA - rva);
    for (uint32_t i = 0; i < exports->mNumNamePointers; i++)
    {
        uint32_t address = exportNames[i];
        if (int(address) > rva && int(address) - rva < int(section.data().size()))
        {
            _exports.addExport(exportOrdinals[i], addresses[i], reinterpret_cast<const char*>(section.data().data() + address - rva));
        }
    }
}

void poPortableExecutable::dumpExports()
{
    _exports.dump();
}
