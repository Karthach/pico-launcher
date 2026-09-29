#pragma once
#include "ff.h"
#include "FastFileRef.h"

class alignas(32) File
{
    FIL _file{};
    bool _isOpen = false;

public:
    FRESULT Open(const TCHAR* path, BYTE mode)
    {
        Close();
        FRESULT result = f_open(&_file, path, mode);
        _isOpen = result == FR_OK;
        return result;
    }

    void Open(const FastFileRef& fastFileRef, BYTE mode)
    {
        Close();
        _file.obj.fs = fastFileRef.GetFatFs();
        _file.obj.id = fastFileRef.GetFatFs()->id;
        _file.obj.sclust = fastFileRef.GetStartCluster();
        _file.obj.attr = 0;
        _file.obj.objsize = fastFileRef.GetFileSize();
        _file.dir_sect = fastFileRef.GetDirSector();
        _file.dir_ptr = &fastFileRef.GetFatFs()->win[fastFileRef.GetDirSectorOffset()];
#if FF_USE_FASTSEEK
        _file.cltbl = 0;
#endif
        _file.flag = mode;
        _file.err = 0;
        _file.sect = 0;
        _file.fptr = 0;
        _isOpen = true;
    }

    FRESULT Close()
    {
        if (!_isOpen)
            return FR_OK;
        FRESULT result = f_close(&_file);
        _isOpen = false;
        return result;
    }

    FRESULT Read(void* buff, u32 count, u32& bytesRead)
    {
        if (!_isOpen)
        {
            bytesRead = 0;
            return FR_INVALID_OBJECT;
        }
        return f_read(&_file, buff, count, (UINT*)&bytesRead);
    }

    bool ReadExact(void* buff, u32 count)
    {
        if (!_isOpen)
            return false;
        UINT bytesRead = 0;
        return f_read(&_file, buff, count, &bytesRead) == FR_OK && bytesRead == count;
    }

    FRESULT Write(const void* buff, u32 count, u32& bytesWritten)
    {
        if (!_isOpen)
        {
            bytesWritten = 0;
            return FR_INVALID_OBJECT;
        }
        return f_write(&_file, buff, count, (UINT*)&bytesWritten);
    }

    FRESULT Seek(FSIZE_t offset) { return _isOpen ? f_lseek(&_file, offset) : FR_INVALID_OBJECT; }
    FRESULT Truncate() { return _isOpen ? f_truncate(&_file) : FR_INVALID_OBJECT; }
    FRESULT Sync() { return _isOpen ? f_sync(&_file) : FR_INVALID_OBJECT; }

    bool IsEof() { return _isOpen && f_eof(&_file); }
    FSIZE_t GetOffset() { return _isOpen ? f_tell(&_file) : 0; }
    FSIZE_t GetSize() { return _isOpen ? f_size(&_file) : 0; }
    FRESULT Rewind() { return _isOpen ? f_rewind(&_file) : FR_INVALID_OBJECT; }

    FRESULT CreateClusterTable(DWORD* clusterTable, u32 byteSize)
    {
        if (!_isOpen)
            return FR_INVALID_OBJECT;
        clusterTable[0] = byteSize / sizeof(DWORD);
        _file.cltbl = clusterTable;
        return f_lseek(&_file, CREATE_LINKMAP);
    }

    File() = default;
    ~File() { Close(); }
};
