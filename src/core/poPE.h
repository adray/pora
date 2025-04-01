#pragma once
#include <string>
#include <vector>

namespace po
{
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

    class poPortableExecutableImportTable
    {
    public:
        void addImport();

    private:

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
        void write(const std::string& filename);
        bool open(const std::string& filename);
        void addSection(const poPortableExecutableSection& section);
        void addDataSections();

        void dumpExports();

    private:
        void genImports(const int imagePos, poPortableExecutableSection& section, const poPortableExecutableData& data);

        void getExports(const int exportIndex, const int rva);
        void findExportTable(const std::vector<int>& sectionIds, int* exportIndex, int* headerId);

        std::vector<poPortableExecutableSection> _sections;
        std::vector<poPortableExecutableData> _dataSections;
        poPortableExecutableExportTable _exports;
        int _textSection;
        int _initializedSection;
        int _uninitializedSection;
        int _readonlySection;
        int _iData;
    };
}
