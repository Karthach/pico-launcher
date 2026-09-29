#pragma once
#include "../IRomBrowserController.h"
#include "services/localization/ILocalizationService.h"
#include "services/settings/RomBrowserDisplaySettings.h"

#include "services/settings/IAppSettingsService.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    DisplaySettingsViewModel(IRomBrowserController* romBrowserController, IAppSettingsService* appSettingsService,
        ILocalizationService* localizationService)
        : _romBrowserController(romBrowserController)
        , _appSettingsService(appSettingsService)
        , _localizationService(localizationService)
        , _romBrowserDisplaySettings(_romBrowserController->GetRomBrowserDisplaySettings()) { }

    constexpr RomBrowserLayout GetRomBrowserDisplayMode() const
    {
        return _romBrowserDisplaySettings.layout;
    }

    void SetRomBrowserDisplayMode(RomBrowserLayout romBrowserDisplayMode)
    {
        if (_romBrowserDisplaySettings.layout != romBrowserDisplayMode)
        {
            _romBrowserDisplaySettings.layout = romBrowserDisplayMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    constexpr RomBrowserSortMode GetRomBrowserSortMode() const
    {
        return _romBrowserDisplaySettings.sortMode;
    }

    void SetRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
    {
        if (_romBrowserDisplaySettings.sortMode != romBrowserSortMode)
        {
            _romBrowserDisplaySettings.sortMode = romBrowserSortMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    const char* GetLanguage() const
    {
        return _appSettingsService->GetAppSettings().language.GetString();
    }

    void SetLanguage(const char* language)
    {
        if (strcmp(_appSettingsService->GetAppSettings().language.GetString(), language) != 0)
        {
            _appSettingsService->GetAppSettings().language = language;
            _appSettingsService->Save();
            _localizationService->Reload();
            _romBrowserController->RestartForLanguageChange();
        }
    }

    void Close()
    {
        _romBrowserController->HideDisplaySettings();
    }

    void GotoSettingsScreen()
    {
        _romBrowserController->GotoSettingsScreen();
    }

private:
    IRomBrowserController* _romBrowserController;
    IAppSettingsService* _appSettingsService;
    ILocalizationService* _localizationService;
    RomBrowserDisplaySettings _romBrowserDisplaySettings;
};
