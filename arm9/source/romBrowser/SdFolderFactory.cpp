#include "common.h"
#include <limits.h>
#include <vector>
#include "fat/Directory.h"
#include "FileInfo.h"
#include "FileType/Folder/FolderFileType.h"
#include "SdFolderFactory.h"

std::unique_ptr<SdFolder> SdFolderFactory::CreateFromPath(const char* path) const
{
    Directory directory;
    if (directory.Open(path) != FR_OK)
        return nullptr;

    int count = 0;
    int bufferSize = 8;
    auto fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * bufferSize);
    if (!fileInfos)
        return nullptr;

    auto releaseFileInfos = [&fileInfos, &count]()
    {
        for (int i = 0; i < count; i++)
            delete fileInfos[i];
        free(fileInfos);
    };
    auto sdFileInfo = std::make_unique<FILINFO>();
    while (true)
    {
        if (directory.Read(sdFileInfo.get()) != FR_OK)
        {
            releaseFileInfos();
            return nullptr;
        }

        if (sdFileInfo->fname[0] == 0)
            break;

        if (count >= bufferSize)
        {
            if (bufferSize > INT_MAX / 2)
            {
                releaseFileInfos();
                return nullptr;
            }
            int newBufferSize = bufferSize * 2;
            auto resizedFileInfos = (FileInfo**)realloc(fileInfos, sizeof(FileInfo*) * newBufferSize);
            if (!resizedFileInfos)
            {
                releaseFileInfos();
                return nullptr;
            }
            fileInfos = resizedFileInfos;
            bufferSize = newBufferSize;
        }
        auto fileType = sdFileInfo->fattrib & AM_DIR
            ? &FolderFileType::sInstance
            : _fileTypeProvider->GetFileType(sdFileInfo->fname);
        fileInfos[count++] = new FileInfo(sdFileInfo->fname, fileType,
            FastFileRef(directory.GetFatFsDirectory(), sdFileInfo.get()), sdFileInfo->fattrib);
    }

    return std::make_unique<SdFolder>(fileInfos, count);
}
