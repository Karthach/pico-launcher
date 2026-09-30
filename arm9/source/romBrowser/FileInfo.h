#pragma once
#include "fat/ff.h"
#include "fat/FastFileRef.h"
#include "FileType/FileType.h"

class InternalFileInfo;

class FileInfo
{
public:
    FileInfo() { }
    FileInfo(const FileInfo& fileInfo);
    FileInfo(const TCHAR* fileName, const FileType* type, const FastFileRef& fastFileRef, u8 attributes,
        const TCHAR* fullPath = nullptr);

    FileInfo &operator=(FileInfo&& rhs)
    {
        if (this != &rhs)
        {
            _name = std::move(rhs._name);
            _fullPath = std::move(rhs._fullPath);
            _type = rhs._type;
            _fastFileRef = rhs._fastFileRef;
        }

        return *this;
    }

    const TCHAR* GetFileName() const { return _name.get(); }
    const TCHAR* GetFullPath() const { return _fullPath.get(); }
    const FileType* GetFileType() const { return _type; }
    u32 GetFileSize() const { return _fastFileRef.GetFileSize(); }

    InternalFileInfo* CreateInternalFileInfo() const
    {
        return _type->CreateInternalFileInfo(_fastFileRef);
    }

    const FastFileRef& GetFastFileRef() const { return _fastFileRef; }

    bool IsReadOnly() const { return _attributes & AM_RDO; }
    bool IsHidden() const { return _attributes & AM_HID; }
    bool IsSystem() const { return _attributes & AM_SYS; }

private:
    std::unique_ptr<TCHAR[]> _name;
    std::unique_ptr<TCHAR[]> _fullPath;
    const FileType* _type;
    FastFileRef _fastFileRef;
    u8 _attributes;
};
