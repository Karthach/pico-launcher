#include "common.h"
#include <string.h>
#include "FileType/CustomIconInternalFileInfo.h"
#include "FileInfoManager.h"

FileInfoManager::FileInfoManager(std::unique_ptr<const FileInfo*[]> items, u32 itemCount, const ICoverRepository& coverRepository,
    const IIconRepository& iconRepository, const IBannerRepository& bannerRepository)
    : _items(std::move(items)), _itemCount(itemCount)
    , _extraFileInfo(std::make_unique<ExtraFileInfo[]>(itemCount))
    , _coverRepository(coverRepository)
    , _iconRepository(iconRepository)
    , _bannerRepository(bannerRepository) { }

u32 FileInfoManager::GetGameCount() const
{
    u32 gameCount = 0;
    for (u32 i = 0; i < _itemCount; i++)
    {
        if (_items[i]->GetFileType()->GetClassification() == FileTypeClassification::Game)
            gameCount++;
    }
    return gameCount;
}

FileInfoManager::~FileInfoManager()
{
    for (u32 i = 0; i < _itemCount; i++)
    {
        ReleaseFileInfo(i);
    }
}

void FileInfoManager::LoadFileInfo(int index)
{
    static const vu8 neverCancel = 0;
    LoadFileInfo(index, neverCancel);
}

void FileInfoManager::LoadFileInfo(int index, const vu8& cancelRequested)
{
    if (cancelRequested || _extraFileInfo[index].loaded)
        return;

    std::unique_ptr<const InternalFileInfo> internalFileInfo(_items[index]->CreateInternalFileInfo());
    const char* gameCode = internalFileInfo ? internalFileInfo->GetGameCode() : nullptr;

    // A custom banner takes priority over the embedded game metadata.
    if (auto customBanner = _bannerRepository.GetBannerForFile(*_items[index], gameCode))
    {
        internalFileInfo.reset(customBanner);
    }
    else if (auto iconData = _iconRepository.GetIconForFile(*_items[index], gameCode))
    {
        internalFileInfo = std::make_unique<CustomIconInternalFileInfo>(
            std::move(iconData), std::move(internalFileInfo));
    }

    if (cancelRequested)
        return;

    if (!_extraFileInfo[index].fileCover.Lock())
    {
        _extraFileInfo[index].fileCover = SharedPtr(
            _coverRepository.GetCoverForFile(*_items[index], internalFileInfo.get()));
    }

    if (cancelRequested)
    {
        _extraFileInfo[index].fileCover.Reset();
        return;
    }

    _extraFileInfo[index].internalFileInfo = internalFileInfo.release();
    _extraFileInfo[index].loaded = true;
}

void FileInfoManager::ReleaseFileInfo(int index)
{
    _extraFileInfo[index].loaded = false;

    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (internalFileInfo)
    {
        _extraFileInfo[index].internalFileInfo = nullptr;
        delete internalFileInfo;
    }

    _extraFileInfo[index].fileCover.Reset();
}

int FileInfoManager::GetItemIndex(const char* fileName)
{
    if (fileName == nullptr)
    {
        return -1;
    }
    for (u32 i = 0; i < _itemCount; i++)
    {
        if (strcmp(fileName, _items[i]->GetFileName()) == 0)
        {
            return i;
        }
    }
    return -1;
}
