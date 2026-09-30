#pragma once
#include <memory>
#include <vector>
#include "core/String.h"
#include "SdFolder.h"
#include "FileType/IFileTypeProvider.h"

class SdFolderFactory
{
public:
    SdFolderFactory(const IFileTypeProvider* fileTypeProvider)
        : _fileTypeProvider(fileTypeProvider) { }

    std::unique_ptr<SdFolder> CreateFromPath(const char* path) const;
    std::unique_ptr<SdFolder> CreateFavorites(const std::vector<String<char, 256>>& paths) const;

private:
    const IFileTypeProvider* _fileTypeProvider;
};
