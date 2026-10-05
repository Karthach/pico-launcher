#include "common.h"
#include <string.h>
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "romBrowser/SdFolderFactory.h"
#include "ThemeRepository.h"

void ThemeRepository::Initialize()
{
    NullFileTypeProvider fileTypeProvider;
    _themesFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/themes");
    if (!_themesFolder)
    {
        _themeFolders.reset();
        _numberOfThemes = 0;
        LOG_ERROR("Theme directory is missing or unreadable.\n");
        return;
    }
    auto filterSortParams = SdFolderFilterSortParams(SdFolderSortType::Name, SdFolderSortDirection::Ascending, false);
    u32 candidateCount = 0;
    auto candidates = _themesFolder->FilterAndSort(filterSortParams, candidateCount);
    auto validThemeFolders = std::make_unique<const FileInfo*[]>(candidateCount);
    _numberOfThemes = 0;
    for (u32 i = 0; i < candidateCount; i++)
    {
        if (_themeInfoFactory.CreateFromThemeFolder(candidates[i]->GetFileName()))
            validThemeFolders[_numberOfThemes++] = candidates[i];
        else
            LOG_ERROR("Skipping theme '%s': theme.json is missing or invalid.\n", candidates[i]->GetFileName());
    }
    _themeFolders = std::move(validThemeFolders);
}

u32 ThemeRepository::GetThemeCount() const
{
    return _numberOfThemes;
}

std::unique_ptr<ThemeInfo> ThemeRepository::LoadThemeInfo(u32 themeIndex) const
{
    if (themeIndex >= _numberOfThemes)
    {
        return nullptr;
    }

    return _themeInfoFactory.CreateFromThemeFolder(_themeFolders[themeIndex]->GetFileName());
}

const TCHAR* ThemeRepository::GetThemeFolderName(int themeIndex) const
{
    if (themeIndex < 0 || (u32)themeIndex >= _numberOfThemes)
    {
        return nullptr;
    }

    return _themeFolders[themeIndex]->GetFileName();
}

int ThemeRepository::FindThemeIndex(const TCHAR* folderName) const
{
    if (folderName == nullptr)
    {
        return -1;
    }
    for (u32 i = 0; i < _numberOfThemes; i++)
    {
        if (strcasecmp(folderName, _themeFolders[i]->GetFileName()) == 0)
        {
            return i;
        }
    }
    return -1;
}
