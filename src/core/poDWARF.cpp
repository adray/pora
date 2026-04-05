#include "poDWARF.h"
#include "poDWARFMachine.h"
#include <cstring>
#include <iostream>

using namespace po;

struct po_DW_CompilationUnit_Header32 {
    uint32_t unit_length;
    uint16_t version;
    uint8_t unit_type; // V5 and onwards
    uint32_t debug_abbr_offset;
    uint8_t addr_size;
};

struct po_DW_LineNumber_Header32 {
    uint32_t unit_length;
    uint16_t version;
    uint32_t header_length;
    uint8_t min_instruction_length;
    uint8_t default_is_stmt;
    int8_t line_base;
    uint8_t line_range;
    uint8_t opcode_base;
};

//
// poDWARF_Buffer
//

int8_t poDWARF_Buffer::readS8() {
    return int8_t(_data[_pos++]);
}

uint64_t poDWARF_Buffer:: readU64() {
    const uint64_t value = 
        _data[_pos] |
        (uint64_t(_data[_pos+1]) << 8) |
        (uint64_t(_data[_pos+2]) << 16) |
        (uint64_t(_data[_pos+3]) << 24) |
        (uint64_t(_data[_pos+4]) << 32) |
        (uint64_t(_data[_pos+5]) << 40) |
        (uint64_t(_data[_pos+6]) << 48) |
        (uint64_t(_data[_pos+7]) << 56);
    _pos += 8;
    return value;
}

uint32_t poDWARF_Buffer:: readU32() {
    const uint32_t value = 
        _data[_pos] |
        (uint32_t(_data[_pos+1]) << 8) |
        (uint32_t(_data[_pos+2]) << 16) |
        (uint32_t(_data[_pos+3]) << 24);
    _pos += 4;
    return value;
}

uint16_t poDWARF_Buffer:: readU16() {
    const uint16_t value = 
        _data[_pos] |
        (uint16_t(_data[_pos+1]) << 8);
    _pos += 2;
    return value;
}
 
uint8_t poDWARF_Buffer:: readU8() {
    const uint8_t value = 
        _data[_pos];
    _pos++;
    return value;
} 

int32_t poDWARF_Buffer:: readLEB128() {
    int32_t value = 0;
    int shift = 0;
    bool ok = true;
    uint8_t byte = 0; 
    while (ok) {
        byte = _data[_pos++];
        value |= (int32_t(byte & 0x7F) << shift);
        shift += 7;
        if (((byte >> 7) & 0x1) == 0) {
            ok = false;
        }

    }

    if (size_t(shift) < sizeof(int32_t) && ((byte & 0x40) != 0x0)) {
        // Sign extend
        value |= (~0 << shift);
    }

    return value;
}

uint32_t poDWARF_Buffer:: readULEB128() {
    uint32_t value = 0;
    int shift = 0;
    bool ok = true;
    while (ok) {
        const uint8_t byte = _data[_pos++];
        value |= (uint32_t(byte & 0x7F) << shift);
        if (((byte >> 7) & 0x1) == 0) {
            ok = false;
        }
        shift += 7;
    }

    return value;
}

void poDWARF_Buffer::readString(std::string& str) {
    char ch = _data[_pos++];
    while (ch != 0) {
         str += ch;
         ch = _data[_pos++];
    }
}

void poDWARF_Buffer::readData(std::vector<uint8_t>& arr, const int count) {
    if (int(arr.capacity()) < count) {
        arr.resize(count);
    }

    std::memcpy(arr.data(), _data.data() + _pos, count); 
}


void poDWARF_Buffer::writeData(unsigned char* data, int size) {
    const int curSize = _data.size();
    if (_data.size() + size_t(size) >= _data.capacity()) {
        _data.resize(_data.size() + size_t(size));
    }

    std::memcpy(_data.data() + curSize, data, size);
}

void poDWARF_Buffer::write(uint32_t value) {
    _data.push_back(value & 0xFF);
    _data.push_back((value >> 8) & 0xFF);
    _data.push_back((value >> 16) & 0xFF);
    _data.push_back((value >> 24) & 0xFF);
}

void poDWARF_Buffer::write(uint16_t value) {
    _data.push_back(value & 0xFF);
    _data.push_back((value >> 8) & 0xFF);
}

void poDWARF_Buffer::write(uint8_t value) {
    _data.push_back(value);
}

void poDWARF_Buffer::write(int8_t value) {
    _data.push_back(*reinterpret_cast<unsigned char*>(&value));
}


//
// poDWARF
//

void poDWARF::write() {

    writeLineInfo();
    writeDebugInfo();
    
}

void poDWARF::read_debug_info(unsigned char* data, const int size) {
    _debug_info.writeData(data, size);
}

void poDWARF::read_debug_line(unsigned char* data, const int size) {
    _debug_line.writeData(data, size);    
}

void poDWARF::read_debug_abbr(unsigned char* data, const int size) {
    _debug_abbr.writeData(data, size);    
}

void poDWARF::read_debug_str(unsigned char* data, const int size) {
    _debug_str.writeData(data, size);
}
 
void poDWARF::read_debug_line_str(unsigned char* data, const int size) {
    _debug_line_str.writeData(data, size);
}
 
void poDWARF::dumpLineInformation() {
    const uint32_t unit_length = _debug_line.readU32();
    const uint16_t version = _debug_line.readU16();
    const uint8_t address_size = _debug_line.readU8();
    const uint8_t segment_selector = _debug_line.readU8();
    const uint32_t header_length = _debug_line.readU32();
    const uint8_t min_instruction_length = _debug_line.readU8();
    const uint8_t max_ops_per_instruction = _debug_line.readU8();
    const uint8_t default_is_statement = _debug_line.readU8();
    const int8_t line_base = _debug_line.readS8();
    const uint8_t line_range = _debug_line.readU8();
    const uint8_t opcode_base = _debug_line.readU8();

    std::vector<uint8_t> standard_opcode_lengths;
    //_debug_line.readArray(standard_opcode_lengths);
    for (int i = 0; i < int(opcode_base) - 1; i++) {
        standard_opcode_lengths.push_back(_debug_line.readU8());
    }

    std::cout << "Unit length: " << unit_length << std::endl
        << "Version: " << version << std::endl
        << "Address size: " << int(address_size) << std::endl
        << "Segment selector: " << int(segment_selector) << std::endl
        << "Header length: " << header_length << std::endl
        << "Min ins length: " << int(min_instruction_length) << std::endl
        << "Max ops per ins: " << int(max_ops_per_instruction) << std::endl
        << "Default is stmt: " << int(default_is_statement) << std::endl
        << "Line base: " << int(line_base) << std::endl
        << "Opcode base: " << int(opcode_base) << std::endl;

    std::vector<uint32_t> content_codes;
    std::vector<uint32_t> form_codes;

    const uint8_t directory_entry_format_count = _debug_line.readU8();
    for (int i = 0; i < int(directory_entry_format_count); i++) {
        const uint32_t content_type_code = _debug_line.readULEB128();
        const uint32_t form_code = _debug_line.readULEB128();

        std::cout << "Dir Entry: " << content_type_code << " " << std::hex << form_code << std::dec << std::endl;
        content_codes.push_back(content_type_code);
        form_codes.push_back(form_code);
    }

    const uint32_t directories_count = _debug_line.readULEB128();
    dumpLineSources(int(directories_count), content_codes, form_codes);

    std::vector<uint32_t> file_content_codes;
    std::vector<uint32_t> file_form_codes;

    const uint8_t file_name_entry_format_count = _debug_line.readU8();
    for (int i = 0; i < int(file_name_entry_format_count); i++) {
        const uint32_t content_type_code = _debug_line.readULEB128();
        const uint32_t form_code = _debug_line.readULEB128();

        std::cout << "File Entry: " << content_type_code << " " << std::hex << form_code << std::dec << std::endl;
        file_content_codes.push_back(content_type_code);
        file_form_codes.push_back(form_code);
    }

    const uint32_t file_names_count = _debug_line.readULEB128();
    dumpLineSources(int(file_names_count), file_content_codes, file_form_codes); 

    _debug_line.seek(int(header_length) + 12);

    std::vector<uint8_t> program;
    _debug_line.readData(program, unit_length - 4 - _debug_line.pos());

    poDWARFMachine machine;
    machine.load(program.data(), int(program.size()));
    machine.run(default_is_statement == 1,
            int(opcode_base), int(line_base), int(line_range),
            int(min_instruction_length),
            int(max_ops_per_instruction));
}

void poDWARF:: dumpLineSources(
        const int source_count, 
        const std::vector<uint32_t>& content_codes,
        const std::vector<uint32_t>& form_codes) {
    for (int i = 0; i < source_count; i++) {
        const int count = int(content_codes.size());
        for (int j = 0; j < count; j++) {
            const uint32_t code = content_codes[j];
            const uint32_t form = form_codes[j];
            if (code == 1 &&
                form == int(poDW_Form::Line_strp)) { // DW_LNCT_PATH
                const uint32_t pos = _debug_line.readU32();
                _debug_line_str.seek(int(pos));
                std::string dir;
                _debug_line_str.readString(dir);
                std::cout << dir << std::endl;
            } else if (code == 2 &&
                    form == int(poDW_Form::UData)) {
                const uint32_t index = _debug_line.readULEB128();
                std::cout << index << std::endl;
            } else {
                // Not implemented code type
            }
        }
    }
}


void poDWARF::dump() {
    // Read abbr table
    for (int i = 0; i < 200; i++) {
        uint32_t id = _debug_abbr.readULEB128();
        uint32_t tag = _debug_abbr.readULEB128();
        uint8_t hasChildren = _debug_abbr.readU8();

        poDWARF_Abbr& abbr = _abbrs.emplace_back(id, tag);
    
        //std::cout << id << " 0x" << std::hex << tag << std::dec << " " << int(hasChildren) << std::endl;
 
        uint32_t attName = _debug_abbr.readULEB128();
        uint32_t attForm = _debug_abbr.readULEB128();
        while (attName != 0 || attForm != 0) {
            //std::cout << std::hex << "0x" << attName << " 0x" << attForm << std::dec << std::endl;

            abbr.attributes().push_back(poDWARF_Attribute(attName, attForm));

            attName = _debug_abbr.readULEB128();
            attForm = _debug_abbr.readULEB128();
        }

        _abbrsMap.insert(std::pair<int, int>(int(id), i));
    }

    // Read compilation unit header
 
    po_DW_CompilationUnit_Header32 header={};
    header.unit_length = _debug_info.readU32();
    header.version = _debug_info.readU16();
    if (header.version >= 5) {
        header.unit_type = _debug_info.readU8();
    }
    header.addr_size = _debug_info.readU8();
    header.debug_abbr_offset = _debug_info.readU32();

    std::cout << "Unit Length: " << header.unit_length << std::endl;
    std::cout << "Version: " << header.version << std::endl;
    std::cout << "Unit Type: " << int(header.unit_type) << std::endl;
    std::cout << "Addr Size: " << int(header.addr_size) << std::endl;
    std::cout << "Debug Abbr Offset: " << int(header.debug_abbr_offset) << std::endl;

    // There may be padding before this entry, but we will ignore this until needed
    
    for (int i = 0 ; i < 20; i++) {
        const uint32_t abbrCode = _debug_info.readULEB128();
        //std::cout << "Abbr code: 0x" << std::hex << abbrCode << std::endl;

        const auto& it = _abbrsMap.find(abbrCode);
        if (it != _abbrsMap.end()) {
            const poDWARF_Abbr& item = _abbrs[it->second];
            std::cout << "Tag: 0x" << std::hex << item.tag() << std::dec << std::endl;
            switch (poDW_Tag(item.tag())) {
                case poDW_Tag::CompilationUnit:
                    std::cout << "Compilation unit:" << std::endl;
                    break;
                case poDW_Tag::StructureType:
                    std::cout << "Structure type:" << std::endl;
                    break;
                case poDW_Tag::TypeDef:
                    std::cout << "TypeDef:" << std::endl;
                    break;
                case poDW_Tag::FormalParameter:
                    std::cout << "Formal param:" << std::endl;
                    break;
                case poDW_Tag::ImportedDecl:
                    std::cout << "Imported decl:" << std::endl;
                    break;
                case poDW_Tag::BaseType:
                    std::cout << "Base type:" << std::endl;
                    break;
                case poDW_Tag::ConstType:
                    std::cout << "Const type:" << std::endl;
                    break;
            }

            for (const poDWARF_Attribute& attr : item.attributes()) {
                std::cout << std::hex << "Attribute=0x" << attr._attribute << " Form=0x" << attr._form << std::dec << std::endl;

                int strPos = 0;
                std::string str;
                switch (poDW_Form(attr._form)) {
                    case poDW_Form::Address:
                        if (header.addr_size == 8) {
                            std::cout << _debug_info.readU64() << std::endl;
                        }
                        break;
                    case poDW_Form::Strp:       // debug_str
                        strPos = int(_debug_info.readU32());
                        if (item.tag() < 0x4000) {
                            _debug_str.seek(strPos);
                            _debug_str.readString(str);
                            std::cout << str << std::endl;
                        }
                        break;
                    case poDW_Form::Line_strp:  // debug_line_str
                        strPos = int(_debug_info.readU32());
                        if (item.tag() < 0x4000) {
                            _debug_line_str.seek(strPos);
                            _debug_line_str.readString(str);
                            std::cout << str << std::endl;
                        }
                        break;
                    case poDW_Form::Ref4:
                        std::cout << int(_debug_info.readU32()) << std::endl;
                        break;
                    case poDW_Form::Data1: // may be signed or unsigned
                        std::cout << int(_debug_info.readU8()) << std::endl;
                        break;
                    case poDW_Form::Data2: // may be signed or unsigned
                        std::cout << int(_debug_info.readU16()) << std::endl;
                        break;
                    case poDW_Form::Data4: // may be signed or unsigned
                        std::cout << _debug_info.readU32() << std::endl;
                        break;
                    case poDW_Form::Sec_offset:
                        if (poDW_AT(attr._attribute) == poDW_AT::Ranges) {
                            std::cout << _debug_info.readU32() << std::endl;
                        } else if (poDW_AT(attr._attribute) == poDW_AT::StatementList) {
                            std::cout << _debug_info.readU32() << std::endl;
                        } else {
                            std::cout << "ERROR" << std::endl;
                        }
                        break;
                    defualt:
                        std::cout << "ERROR" << std::endl;
                        break;
                }
            }
        }
    }

    dumpLineInformation();
}

void poDWARF::writeDebugInfo() {

    // Write compilation unit header

    po_DW_CompilationUnit_Header32 header;
    header.unit_length = sizeof(header) - sizeof(uint32_t); // total size of compilation unit (not including unit length field)
    header.version = 5;
    header.debug_abbr_offset = 0;
    header.addr_size = 0;

    _debug_info.write(header.unit_length);
    _debug_info.write(header.version);
    _debug_info.write(header.debug_abbr_offset);
    _debug_info.write(header.addr_size);
}

void poDWARF::writeLineInfo() {
    
    // Write line info header (pg 107 of DWARF 3 spec)

    po_DW_LineNumber_Header32 header = {};
    header.unit_length = sizeof(header) - sizeof(uint32_t);
    header.version = 0; // TODO: Appendix F
    header.header_length = sizeof(header) - sizeof(uint32_t) - sizeof(uint16_t);
    header.min_instruction_length = 1;
    header.default_is_stmt = 0;
    header.line_base = 0;
    header.line_range = 0;
    header.opcode_base = 0;
}

