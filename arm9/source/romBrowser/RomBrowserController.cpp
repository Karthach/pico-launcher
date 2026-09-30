#include "common.h"
#include <array>
#include "App.h"
#include "services/process/ProcessManager.h"
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, TaskQueueBase* ioTaskQueue,
    TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings()) { }

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::GotoSettingsScreen()
{
    _stateMachine.Fire(RomBrowserStateTrigger::GotoSettingsScreen);
}

void RomBrowserController::RestartForLanguageChange()
{
    TCHAR currentPath[256];
    if (f_getcwd(currentPath, sizeof(currentPath) / sizeof(currentPath[0])) == FR_OK)
    {
        _appSettingsService->GetAppSettings().lastUsedFilePath = currentPath;
        _appSettingsService->Save();
    }
    gProcessManager.Goto<App>();
}

void RomBrowserController::SetSearchQuery(const char* query)
{
    if (strcmp(_searchQuery.GetString(), query) == 0)
        return;
    _searchQuery = query;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

static bool BuildFavoritePath(char* path, size_t pathSize, const char* folder, const char* fileName)
{
    if (!folder || !fileName)
        return false;
    StringUtil::Copy(path, folder, pathSize);
    size_t length = strlen(path);
    if (length + 1 >= pathSize)
        return false;
    if (length == 0 || path[length - 1] != '/')
        path[length++] = '/';
    path[length] = 0;
    if (strlen(fileName) >= pathSize - length)
        return false;
    strlcat(path, fileName, pathSize);
    return true;
}

bool RomBrowserController::IsFavorite(const FileInfo& fileInfo) const
{
    char path[512];
    if (!BuildFavoritePath(path, sizeof(path), _currentFolderPath.GetString(), fileInfo.GetFileName()) || strlen(path) > 256)
        return false;
    const auto& favorites = _appSettingsService->GetAppSettings().favoritePaths;
    for (const auto& favorite : favorites)
        if (!strcasecmp(favorite.GetString(), path))
            return true;
    return false;
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo)
{
    char path[512];
    if (!BuildFavoritePath(path, sizeof(path), _currentFolderPath.GetString(), fileInfo.GetFileName()) || strlen(path) > 256)
        return;
    auto& favorites = _appSettingsService->GetAppSettings().favoritePaths;
    for (auto it = favorites.begin(); it != favorites.end(); ++it)
    {
        if (!strcasecmp(it->GetString(), path))
        {
            favorites.erase(it);
            _appSettingsService->Save();
            if (_favoritesView) _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
            return;
        }
    }
    if (favorites.size() < 10)
        favorites.emplace_back(path);
    _appSettingsService->Save();
    if (_favoritesView) _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::ToggleFavoritesView()
{
    _favoritesView = !_favoritesView;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::Update()
{
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            const auto& lastUsed = _appSettingsService->GetAppSettings().lastUsedFilePath;
            if (strlen(lastUsed.GetString()) != 0)
            {
                NavigateToPath(lastUsed.GetString());
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        case RomBrowserStateTrigger::GotoSettingsScreen:
            HandleGotoSettingsScreenTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_iconRepository)
        {
            _iconRepository = std::make_unique<IconRepository>();
            _iconRepository->Initialize();
        }
        if (!_bannerRepository)
        {
            _bannerRepository = std::make_unique<BannerRepository>();
            _bannerRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        u64 startTick = gTickCounter.GetValue();
        _navigateFileName = nullptr;
        if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
        {
            FILINFO fileInfo;
            if (f_stat(_navigatePath, &fileInfo) != FR_OK)
            {
                StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
            }
            else if (!(fileInfo.fattrib & AM_DIR))
            {
                _navigateFileName = strrchr(_navigatePath, '/') + 1;
                _navigateFileName[-1] = 0;
            }
        }
        f_chdir(_navigatePath);
        TCHAR currentFolderPath[257];
        if (f_getcwd(currentFolderPath, sizeof(currentFolderPath)) == FR_OK)
            _currentFolderPath = currentFolderPath;
        SdFolderFactory sdFolderFactory { &_fileTypeProvider };
        _newSdFolder = sdFolderFactory.CreateFromPath(".");
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        UpdateLastUsedFilepath();
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
    if (_navigatePath[idx - 2] == '/')
    {
        _navigatePath[idx - 1] = 0;
    }
    strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _appSettingsService->Save();
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
