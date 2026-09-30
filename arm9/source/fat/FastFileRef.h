#pragma once
#include "ff.h"

class FastFileRef
{
    FATFS* _fatFs;
    u32 _dirSector;
    u32 _dirSectorOffset;
    u32 _startCluster;
    FSIZE_t _fileSize;
    WORD _fileDate = 0;
    WORD _fileTime = 0;

public:
    FastFileRef() { }

    explicit FastFileRef(const DIR* directory, const FILINFO* fileInfo)
        : _fatFs(directory->obj.fs), _dirSector(fileInfo->fdirsect)
        , _dirSectorOffset(fileInfo->fdiroffs), _startCluster(fileInfo->fclust)
        , _fileSize(fileInfo->fsize), _fileDate(fileInfo->fdate), _fileTime(fileInfo->ftime) { }

    FATFS* GetFatFs() const { return _fatFs; }
    u32 GetDirSector() const { return _dirSector; }
    u32 GetDirSectorOffset() const { return _dirSectorOffset; }
    u32 GetStartCluster() const { return _startCluster; }
    FSIZE_t GetFileSize() const { return _fileSize; }
    u32 GetLastModifiedTimestamp() const { return ((u32)_fileDate << 16) | _fileTime; }
};
