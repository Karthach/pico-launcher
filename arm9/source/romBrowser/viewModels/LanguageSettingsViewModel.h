#pragma once
#include "../IRomBrowserController.h"
#include "services/settings/IAppSettingsService.h"

class LanguageSettingsViewModel
{
public:
    LanguageSettingsViewModel(IRomBrowserController* romBrowserController, IAppSettingsService* appSettingsService)
        : _romBrowserController(romBrowserController)
        , _appSettingsService(appSettingsService) { }

    const char* GetCurrentLanguage() const
    {
        return _appSettingsService->GetAppSettings().language.GetString();
    }

    void SetLanguage(const char* language)
    {
        if (strcmp(_appSettingsService->GetAppSettings().language.GetString(), language) != 0)
        {
            _appSettingsService->GetAppSettings().language = language;
            _appSettingsService->Save();
            // Need to restart or reload strings. For now just save.
        }
    }

    void Close()
    {
        _romBrowserController->HideLanguageSettings();
    }

private:
    IRomBrowserController* _romBrowserController;
    IAppSettingsService* _appSettingsService;
};
