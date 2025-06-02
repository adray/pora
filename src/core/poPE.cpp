#include "poPE.h"
#include <fstream>
#include <cstdint>
#include <iostream>
#include <string.h>

using namespace po;

/*
* Concepts:
* File pointer: offset relative to the file stored on disk.
* Relative virtual address (RVA): this is the address of an item after it is loaded into memory
* with the base of the address subtracted from it.
* Virtual address (VA): Same as RVA, except the base address is not subtracted from it.
*/

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
    _entryPoint(0)
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

void poPortableExecutable::addSection(const poPortableExecutableSection& section)
{
    switch (section.type())
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
 
    _sections.push_back(section);
}

void poPortableExecutable::genImports(const int imagePos, poPortableExecutableSection& section, const poPortableExecutableData& data)
{
    /* Import Directory entry per DLL */
    const int numDLLs = 1;
    PeImportDirectory* imports = reinterpret_cast<PeImportDirectory*>(section.data().data() + data.offset());
    imports[0].mName = imagePos + data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + 128 /* just some made up offset for now... */;
    imports[0].mCharacteristics = imagePos + data.offset() + sizeof(PeImportDirectory) * (numDLLs+1); /*import lookup table*/
    imports[0].mImportAddressTable = imagePos + data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + 512 /* just some made up offset for now... */;
    /* last import is all NULLs */

    /* Import lookup table */
    uint64_t* apis =  reinterpret_cast<uint64_t*>(imports[0].mCharacteristics - imagePos + section.data().data());
    apis[0] = imagePos + data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + 256 /* just some made up offset for now... */;
    /*Last entry set to NULL*/

    /* Import Directory - Name RVA table */
    char* name = reinterpret_cast<char*>(int(imports[0].mName) - imagePos + section.data().data());
    const char* kernelDll = "KERNEL32.DLL";
    memcpy(name, kernelDll, strlen(kernelDll) + 1);

    /* Hint/Name RVA table */
    short* api_hint = reinterpret_cast<short*>(apis[0] - imagePos + section.data().data());
    char* api_name = reinterpret_cast<char*>(apis[0] - imagePos + section.data().data() + sizeof(short));
    const char* virtualAlloc = "VirtualAlloc";
    memcpy(api_name, virtualAlloc, strlen(virtualAlloc) + 1);

    /* Import address table
    * These are the same of the import lookup table, until it is loaded.
    * Then these are filled with the actual addresses of the functions.
    */
    uint64_t* importAddresses = reinterpret_cast<uint64_t*>(imports[0].mImportAddressTable - imagePos + section.data().data());
    importAddresses[0] = imagePos + data.offset() + sizeof(PeImportDirectory) * (numDLLs + 1) + 256 /* just some made up offset for now... */;


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

void poPortableExecutable::write(const std::string& filename)
{
    if (_textSection == -1 || _uninitializedSection == -1 || _initializedSection == -1 || _iData == -1)
    {
        return;
    }

    std::ofstream stream(filename, std::ios::binary);
    if (stream.is_open())
    {
        const poPortableExecutableSection& section = _sections[_textSection];
        const poPortableExecutableSection& uninitializedSection = _sections[_uninitializedSection];
        const poPortableExecutableSection& initializedSection = _sections[_initializedSection];

        const int numDataDirectories = 16;
        PeImageDataDirectory dirs[numDataDirectories] = {};

        uint8_t dos_magic_number[64] = {};
        uint8_t standard_stub[128] = {};

        PeHeader header = {};
        header.mMagic = 0x50 | (0x45 << 8) | (0x0 << 16) | (0x0 << 24);
        header.mMachine = 0x64 | (0x86 << 8); /* x86_64 */
        header.mNumberOfSections = int16_t(_sections.size());
        header.mTimeDateStamp = 0; /* milliseconds since 1970 1st Jan */
        header.mCharacteristics = 0x22; /* 0x2 = IMAGE_FILE_EXECUTABLE_IMAGE, 0x20 = IMAGE_FILE_LARGE_ADDRESS_AWARE  */
        header.mSizeOfOptionalHeader = sizeof(Pe32PlusOptionalHeader) + sizeof(dirs);

        const int sizeOfHeaders = sizeof(standard_stub) +
            sizeof(dos_magic_number) +
            sizeof(header) +
            sizeof(Pe32PlusOptionalHeader) +
            sizeof(dirs) +
            sizeof(PeSectionTable) * int(_sections.size());

        Pe32PlusOptionalHeader optionalHeader = {};
        optionalHeader.mMagic = 0x020b;
        optionalHeader.mSizeOfCode = uint32_t(section.data().size());
        optionalHeader.mSizeOfUninitializedData = _uninitializedSection > -1 ? uint32_t(uninitializedSection.data().size()) : 0;
        optionalHeader.mSizeOfInitializedData = _initializedSection > -1 ? uint32_t(initializedSection.data().size()) : 0;
        optionalHeader.mMajorOperatingSystemVersion = 0x6;
        optionalHeader.mMajorSubsystemVersion = 0x6;
        optionalHeader.mAddressOfEntryPoint = 0x01000 /*hardcoded to text section..*/ + _entryPoint ;
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

        std::vector<poSection> sectionData;
        for (auto& section : _sections)
        {
            auto& data = sectionData.emplace_back();
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
                genImports(sectionData[_iData]._imagePos, _sections[_iData], section);
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
                    dir.mVirtualAddress = sectionData[_iData]._imagePos + section.offset();
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

        *reinterpret_cast<int32_t*>(dos_magic_number) = 0x5A4D;
        *reinterpret_cast<int32_t*>(dos_magic_number + offset) = sizeof(dos_magic_number) + sizeof(standard_stub);

        // Write headers
        stream.write((char*)dos_magic_number, sizeof(dos_magic_number));
        stream.write((char*)standard_stub, sizeof(standard_stub));
        stream.write((char*)&header, sizeof(header));
        stream.write((char*)&optionalHeader, sizeof(optionalHeader));
        stream.write((char*)dirs, sizeof(dirs));

        const std::string textName = ".text";
        const std::string initName = ".data";
        const std::string uninitializedName = ".bss";
        const std::string iDataName = ".idata";

        // Write section headers
        for (size_t i = 0; i < _sections.size(); i++)
        {
            auto& section = _sections[i];
            auto& data = sectionData[i];

            PeSectionTable table = {};
            switch (section.type())
            {
            case poSectionType::TEXT:
                memcpy(table.mName, textName.data(), textName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
                break;
            case poSectionType::INITIALIZED:
                memcpy(table.mName, initName.data(), initName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_INITIALIZED_DATA /* | IMAGE_SCN_MEM_WRITE*/ | IMAGE_SCN_MEM_READ;
                break;
            case poSectionType::UNINITIALIZED:
                memcpy(table.mName, uninitializedName.data(), uninitializedName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                break;
            case poSectionType::IDATA:
                memcpy(table.mName, iDataName.data(), iDataName.size());
                table.mCharacteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                break;
            }
            table.mVirtualSize = uint32_t(section.data().size());
            table.mVirtualAddress = data._imagePos;
            table.mSizeOfRawData = uint32_t(section.data().size());
            table.mPointerToRawData = data._filePos;

            stream.write((char*)&table, sizeof(table));
        }

        std::vector<char> sectionPadding(optionalHeader.mSizeOfHeaders - sizeOfHeaders);
        stream.write((char*)sectionPadding.data(), sectionPadding.size()); // Write padding

        // Write sections
        for (size_t i = 0; i < _sections.size(); i++)
        {
            auto& section = _sections[i];
            auto& data = sectionData[i];

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
