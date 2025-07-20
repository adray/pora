#pragma once
#include <string>
#include <vector>

namespace po
{
    struct poPEImage;

    enum class poSectionType
    {
        TEXT, /* default section for code (.text) */
        INITIALIZED, /* initialized data, static and global variables (.data) */
        UNINITIALIZED, /* uninitialized static and global variables (.bss) */
        READONLY, /* (.rdata) */
        IDATA /* import data (.idata) */
    };

    enum class poDataDirectoryType
    {
        /* DATA DIRECTORIES */

        EXPORT,
        IMPORT,
        RESOURCE,
        EXCEPTION,
        CERTIFICATE,
        BASE_RELOCATION,
        DEBUG,
        ARCHITECTURE,
        GLOBAL_PTR,
        TLS,
        LOAD_CONFIG,
        BOUND_IMPORT,
        IAT,
        DELAY_IMPORT,
        CLR_RUNTIME,
        RESERVED
    };

    class poPortableExecutableData
    {
    public:
        poPortableExecutableData(poSectionType parent, poDataDirectoryType type, int offset, int size)
            :
            _parent(parent),
            _type(type),
            _offset(offset),
            _size(size)
        { }
        inline poDataDirectoryType type() const { return _type; }
        inline poSectionType parent() const { return _parent; }
        inline int offset() const { return _offset; }
        inline int size() const { return _size; }

    private:
        int _offset;
        int _size;
        poSectionType _parent;
        poDataDirectoryType _type;
    };

    class poPortableExecutableSection
    {
    public:
        poPortableExecutableSection(poSectionType type)
            :
            _type(type)
        { }
        inline std::vector<unsigned char>& data() { return _data; }
        inline const std::vector<unsigned char>& data() const { return _data; }
        inline poSectionType type() const { return _type; }

    private:
        std::vector<unsigned char> _data;
        poSectionType _type;
    };

    class poImportEntry
    {
    public:
        poImportEntry(const std::string& name, int ordinal);

        inline const std::string& name() const { return _name; }
        inline int nameRVA() const { return _nameRVA; }
        inline int addressRVA() const { return _addressRVA; }
        inline int ordinal() const { return _ordinal; }

        inline void setNameRVA(const int rva) { _nameRVA = rva; }
        inline void setAddressRVA(const int rva) { _addressRVA = rva; }

    private:
        std::string _name;
        int _nameRVA; /* RVA to the name of the imported function */
        int _addressRVA; /* RVA to the address of the imported function */
        int _ordinal; /* Ordinal of the imported function */
    };

    class poPortableExecutableImportTable
    {
    public:
        poPortableExecutableImportTable(const std::string& dllName);
        void addImport(const std::string& name, const int ordinal);

        inline const std::string& dllName() const { return _dllName; }
        inline const int importPosition() const { return _importPosition; }

        inline const std::vector<poImportEntry>& imports() const { return _imports; }
        inline std::vector<poImportEntry>& imports() { return _imports; }

        inline void setImportPosition(const int pos) { _importPosition = pos; }

    private:
        int _importPosition;
        std::string _dllName;
        std::vector<poImportEntry> _imports;
    };

    class poPortableExecutableExportTable
    {
    public:
        void addExport(int nameRVA, const char* name);
        void addExport(int ordinal, int addressRVA, const char* name);
        void dump();

    private:
        struct poExportEntry
        {
            union
            {
                int _index;
                int _ordinal;
            };
            union
            {
                int _nameRVA;
                int _addressRVA;
            };
            const char* _name;
        };

        std::vector<poExportEntry> _exports;
        std::vector<poExportEntry> _forwarderExports;
    };

    class poPortableExecutable
    {
    public:
        poPortableExecutable();
        void initializeSections();
        void write(const std::string& filename);
        bool open(const std::string& filename);
        void addSection(const poSectionType type, const int size);

        inline poPortableExecutableSection& textSection() { return _sections[_textSection]; }
        inline poPortableExecutableSection& initializedDataSection() { return _sections[_initializedSection]; }

        poPortableExecutableImportTable& addImportTable(const std::string& name);

        void dumpExports();

        inline void setEntryPoint(const int entryPoint) { _entryPoint = entryPoint; }
        inline const int getEntryPoint() const { return _entryPoint; }

    private:
        void addDataSections();
        
        void genImports(const int imagePos, poPortableExecutableSection& section, const poPortableExecutableData& data);
        void buildImportNameTable(std::vector<unsigned char>& importNameTable);
        void buildImportLookupTable(std::vector<unsigned char>& importLookupTable);
        void buildImportHintTable(std::vector<unsigned char>& importHintTable, const int imageBase);

        void getExports(const int exportIndex, const int rva);
        void findExportTable(const std::vector<int>& sectionIds, int* exportIndex, int* headerId);

        std::vector<poPortableExecutableSection> _sections;
        std::vector<poPortableExecutableData> _dataSections;
        std::vector<poPortableExecutableImportTable> _imports;
        poPortableExecutableExportTable _exports;
        int _textSection;
        int _initializedSection;
        int _uninitializedSection;
        int _readonlySection;
        int _iData;
        int _entryPoint;
        poPEImage* _peImage;
    };
}
