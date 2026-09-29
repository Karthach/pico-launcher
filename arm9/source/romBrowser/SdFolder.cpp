#include "common.h"
#include <string.h>
#include <algorithm>
#include "FileType/InternalFileInfo.h"
#include "SdFolder.h"

static int CompareTitleStrings(const char16_t* a, const char16_t* b)
{
    if (a == b) return 0;
    if (!a) return 1;
    if (!b) return -1;
    while (*a && *b)
    {
        char16_t charA = *a;
        char16_t charB = *b;
        if (charA >= u'A' && charA <= u'Z') charA += u'a' - u'A';
        if (charB >= u'A' && charB <= u'Z') charB += u'a' - u'A';
        if (charA != charB) return (int)charA - (int)charB;
        a++;
        b++;
    }
    return (int)*a - (int)*b;
}

SdFolder::SdFolder(FileInfo** files, int fileCount)
    : _files(files), _fileCount(fileCount) { }

SdFolder::~SdFolder()
{
    for (int i = 0; i < _fileCount; i++)
        delete _files[i];
    free(_files);
}

std::unique_ptr<const FileInfo*[]> SdFolder::FilterAndSort(
    const SdFolderFilterSortParams& filterSortParams, u32& resultCount) const
{
    auto sortedFilteredFiles = std::make_unique<const FileInfo*[]>(_fileCount);
    u32 filteredCount = 0;
    for (int i = 0; i < _fileCount; i++)
    {
        const FileInfo* file = _files[i];
        bool isHidden = file->GetFileName()[0] == '.' || file->IsHidden();
        auto classification = file->GetFileType()->GetClassification();
        if (classification != FileTypeClassification::Unknown &&
            (!isHidden || filterSortParams.includeHiddenFiles))
        {
            sortedFilteredFiles[filteredCount++] = file;
        }
    }

    if (filterSortParams.sortType == SdFolderSortType::Title)
    {
        struct TitleEntry
        {
            const FileInfo* fileInfo;
            std::unique_ptr<char16_t[]> title;
        };

        auto titleEntries = std::make_unique<TitleEntry[]>(filteredCount);
        for (int i = 0; i < filteredCount; i++)
        {
            titleEntries[i].fileInfo = sortedFilteredFiles[i];
            auto internalFileInfo = std::unique_ptr<InternalFileInfo>(
                titleEntries[i].fileInfo->CreateInternalFileInfo());
            const char16_t* title = internalFileInfo ? internalFileInfo->GetGameTitle() : nullptr;
            if (title)
            {
                u32 len = 0;
                while (title[len]) len++;
                titleEntries[i].title = std::make_unique<char16_t[]>(len + 1);
                for (u32 j = 0; j <= len; j++) titleEntries[i].title[j] = title[j];
            }
        }

        std::sort(titleEntries.get(), titleEntries.get() + filteredCount,
            [filterSortParams] (const TitleEntry& a, const TitleEntry& b)
            {
                bool result = true;
                if (CompareClassification(a.fileInfo, b.fileInfo, result))
                    return result;

                if (a.fileInfo->GetFileType()->GetClassification() == FileTypeClassification::Folder &&
                    b.fileInfo->GetFileType()->GetClassification() == FileTypeClassification::Folder)
                {
                    return CompareName(a.fileInfo, b.fileInfo);
                }

                int cmp = CompareTitleStrings(a.title.get(), b.title.get());
                if (cmp == 0)
                {
                    if (CompareName(a.fileInfo, b.fileInfo))
                        cmp = -1;
                    else if (CompareName(b.fileInfo, a.fileInfo))
                        cmp = 1;
                }

                return filterSortParams.sortDirection == SdFolderSortDirection::Ascending
                    ? cmp < 0
                    : cmp > 0;
            });

        for (int i = 0; i < filteredCount; i++)
            sortedFilteredFiles[i] = titleEntries[i].fileInfo;
    }
    else
    {
        std::sort(sortedFilteredFiles.get(), sortedFilteredFiles.get() + filteredCount,
            [filterSortParams] (const FileInfo*& a, const FileInfo*& b)
            {
                bool result = true;
                if (CompareClassification(a, b, result))
                    return result;

                auto sortType = filterSortParams.sortType;
                auto sortDirection = filterSortParams.sortDirection;
                if (a->GetFileType()->GetClassification() == FileTypeClassification::Folder &&
                    b->GetFileType()->GetClassification() == FileTypeClassification::Folder)
                {
                    if (sortType != SdFolderSortType::Name)
                    {
                        sortType = SdFolderSortType::Name;
                        sortDirection = SdFolderSortDirection::Ascending;
                    }
                }
                int cmp = 0;
                switch (sortType)
                {
                    case SdFolderSortType::Name:
                    {
                        if (CompareName(a, b))
                            cmp = -1;
                        else if (CompareName(b, a))
                            cmp = 1;
                        break;
                    }
                    case SdFolderSortType::LastModified:
                    {
                        u32 aTimestamp = a->GetFastFileRef().GetLastModifiedTimestamp();
                        u32 bTimestamp = b->GetFastFileRef().GetLastModifiedTimestamp();
                        if (aTimestamp < bTimestamp)
                            cmp = -1;
                        else if (aTimestamp > bTimestamp)
                            cmp = 1;
                        else if (CompareName(a, b))
                            cmp = -1;
                        else if (CompareName(b, a))
                            cmp = 1;
                        break;
                    }
                    default:
                    {
                        if (CompareName(a, b))
                            cmp = -1;
                        else if (CompareName(b, a))
                            cmp = 1;
                        break;
                    }
                }
                return sortDirection == SdFolderSortDirection::Ascending
                    ? cmp < 0
                    : cmp > 0;
            });
    }
    resultCount = filteredCount;
    return sortedFilteredFiles;
}

bool SdFolder::CompareClassification(const FileInfo* a, const FileInfo* b, bool& result)
{
    auto aClassification = a->GetFileType()->GetClassification();
    auto bClassification = b->GetFileType()->GetClassification();
    if (aClassification == bClassification)
        return false;

    if (aClassification == FileTypeClassification::Folder)
    {
        result = true;
        return true;
    }
    else if (bClassification == FileTypeClassification::Folder)
    {
        result = false;
        return true;
    }

    return false;
}

bool SdFolder::CompareName(const FileInfo* a, const FileInfo* b)
{
    return strcasecmp(a->GetFileName(), b->GetFileName()) < 0;
}

void SdFolder::SortByNameInPlace()
{
    std::sort(_files, _files + _fileCount, CompareName);
}

const FileInfo* SdFolder::BinarySearch(const char* fileName) const
{
    if (_fileCount != 0)
    {
        const auto file = std::lower_bound(_files, _files + _fileCount, fileName,
            [] (const FileInfo* entry, const char* value)
            {
                return strcasecmp(entry->GetFileName(), value) < 0;
            });

        if (file != _files + _fileCount && !strcasecmp((*file)->GetFileName(), fileName))
        {
            return *file;
        }
    }

    return nullptr;
}
