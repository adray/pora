#pragma once

namespace po
{
    class poDebugInfo
    {
        public:
            poDebugInfo() :
                _fileId(0),
                _line(0),
                _col(0)
            {
            }

            poDebugInfo(const int fileId, const int line, const int col) : 
                _fileId(fileId),
                _line(line),
                _col(col)
            {
            }

            int fileId() const { return _fileId; }
            int line() const { return _line; }
            int col() const { return _col; }

        private:
            int _fileId;
            int _line;
            int _col;
    };
}

