#include "common.h"
#include <algorithm>
#include <cctype>
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserViewModel.h"

static bool ContainsText(const char* value, const char* query)
{
    if (!query || !query[0]) return true;
    if (!value) return false;
    const size_t queryLength = strlen(query);
    for (const char* start = value; *start; start++)
    {
        size_t i = 0;
        while (i < queryLength && start[i] &&
            tolower((unsigned char)start[i]) == tolower((unsigned char)query[i]))
            i++;
        if (i == queryLength) return true;
    }
    return false;
}

static bool ContainsTitle(const char16_t* value, const char* query)
{
    if (!query || !query[0]) return true;
    if (!value) return false;
    const size_t queryLength = strlen(query);
    for (const char16_t* start = value; *start; start++)
    {
        size_t i = 0;
        while (i < queryLength && start[i] && start[i] <= 0x7f &&
            tolower((unsigned char)start[i]) == tolower((unsigned char)query[i]))
            i++;
        if (i == queryLength) return true;
    }
    return false;
}

RomBrowserViewModel::RomBrowserViewModel(IRomBrowserController* romBrowserController, const char* initialSelectedFileName)
    : _romBrowserController(romBrowserController)
{
    SdFolderFilterSortParams filterSortParams;
    switch (romBrowserController->GetRomBrowserDisplaySettings().sortMode)
    {
        case RomBrowserSortMode::NameAscending:
        default:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Ascending, false);
            break;
        }
        case RomBrowserSortMode::NameDescending:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Descending, false);
            break;
        }
        case RomBrowserSortMode::LastModified:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::LastModified, SdFolderSortDirection::Descending, false);
            break;
        }
        case RomBrowserSortMode::TitleAscending:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Title, SdFolderSortDirection::Ascending, false);
            break;
        }
        case RomBrowserSortMode::TitleDescending:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Title, SdFolderSortDirection::Descending, false);
            break;
        }
    }
    u64 startTick = gTickCounter.GetValue();
    const auto& sdFolder = romBrowserController->GetSdFolder();
    u32 filteredCount;
    auto sortedFilteredFiles = sdFolder.FilterAndSort(filterSortParams, filteredCount);
    const char* query = romBrowserController->GetSearchQuery();
    const bool favoritesOnly = romBrowserController->IsFavoritesView();
    auto visibleFiles = std::make_unique<const FileInfo*[]>(filteredCount);
    u32 visibleCount = 0;
    for (u32 i = 0; i < filteredCount; i++)
    {
        const FileInfo& file = *sortedFilteredFiles[i];
        if (favoritesOnly && file.GetFileType()->GetClassification() != FileTypeClassification::Folder &&
            !romBrowserController->IsFavorite(file))
            continue;

        bool matches = ContainsText(file.GetFileName(), query);
        if (!matches && query[0])
        {
            auto info = std::unique_ptr<InternalFileInfo>(file.CreateInternalFileInfo());
            matches = ContainsTitle(info ? info->GetGameTitle() : nullptr, query);
        }
        if (matches)
            visibleFiles[visibleCount++] = &file;
    }
    u64 endTick = gTickCounter.GetValue();
    LOG_DEBUG("Filter + sort took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
    _fileInfoManager = std::make_unique<FileInfoManager>(std::move(visibleFiles),
        visibleCount, _romBrowserController->GetCoverRepository(), _romBrowserController->GetIconRepository(),
        _romBrowserController->GetBannerRepository());
    _selectedItem = _fileInfoManager->GetItemIndex(initialSelectedFileName);
}

void RomBrowserViewModel::NavigateUp()
{
    _romBrowserController->NavigateUp();
}
