#pragma once
#include "../IRomBrowserController.h"
#include "fat/Directory.h"
#include "core/String.h"
#include "services/settings/IAppSettingsService.h"

class ThemeSettingsViewModel
{
public:
    explicit ThemeSettingsViewModel(IRomBrowserController* romBrowserController, IAppSettingsService* appSettingsService)
        : _romBrowserController(romBrowserController)
        , _appSettingsService(appSettingsService)
        , _themeCount(0)
    {
        RefreshThemes();
    }

    void RefreshThemes()
    {
        _themeCount = 0;
        Directory directory;
        if (directory.Open("/_pico/themes") == FR_OK)
        {
            FILINFO fileInfo;
            while (directory.Read(&fileInfo) == FR_OK && fileInfo.fname[0] != 0 && _themeCount < 8)
            {
                if (fileInfo.fattrib & AM_DIR)
                {
                    if (fileInfo.fname[0] == '.') continue;
                    _themes[_themeCount++] = fileInfo.fname;
                }
            }
        }
    }

    int GetThemeCount() const { return _themeCount; }
    const char* GetTheme(int index) const { return _themes[index].GetString(); }

    const char* GetCurrentTheme() const
    {
        return _appSettingsService->GetAppSettings().theme.GetString();
    }

    void SetTheme(const char* theme)
    {
        if (strcmp(theme, GetCurrentTheme()) != 0)
        {
            _appSettingsService->GetAppSettings().theme = theme;
            _appSettingsService->Save();
            // Trigger app reload
            _romBrowserController->SetRomBrowserDisplaySettings(_appSettingsService->GetAppSettings().romBrowserDisplaySettings);
        }
    }

    void Close()
    {
        _romBrowserController->HideThemeSettings();
    }

private:
    IRomBrowserController* _romBrowserController;
    IAppSettingsService* _appSettingsService;
    String<char, 64> _themes[8];
    int _themeCount;
};
