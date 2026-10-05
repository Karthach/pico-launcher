#pragma once
#include <string.h>
#include "../IRomBrowserController.h"
#include "services/settings/IAppSettingsService.h"
#include "services/localization/ILocalizationService.h"

/// @brief View model for the menu the app bar's "more" button opens. Every
///        entry forwards to the controller: the panels replace the sheet with
///        their own, and a filter closes it through the display mode change
///        it fires (see the Menu state in RomBrowserStateMachine).
class MenuViewModel
{
public:
    MenuViewModel(IRomBrowserController* romBrowserController, IAppSettingsService* appSettingsService,
        ILocalizationService* localizationService)
        : _romBrowserController(romBrowserController), _appSettingsService(appSettingsService),
          _localizationService(localizationService) { }

    void ShowRecents() { _romBrowserController->ShowRecents(); }
    void ShowFavorites() { _romBrowserController->ShowFavorites(); }
    void ShowStatistics() { _romBrowserController->ShowStatistics(); }
    bool CanDeleteSelected() const { return _romBrowserController->CanDeleteSelected(); }
    void RequestDeleteSelected() { _romBrowserController->RequestDeleteSelected(); }
    void ToggleFavoritesFilter() { _romBrowserController->ToggleFavoritesFilter(); }
    bool IsFavoritesFilterEnabled() const { return _romBrowserController->IsFavoritesFilterEnabled(); }
    void ToggleCompletedFilter() { _romBrowserController->ToggleCompletedFilter(); }
    bool IsCompletedFilterEnabled() const { return _romBrowserController->IsCompletedFilterEnabled(); }

    void ShowAbout() { _romBrowserController->ShowAbout(); }
    const char* GetLanguageCode() const { return _appSettingsService->GetAppSettings().language.GetString(); }
    void ToggleLanguage()
    {
        auto& language = _appSettingsService->GetAppSettings().language;
        language = strcmp(language.GetString(), "spanish") == 0 ? "english" : "spanish";
        _appSettingsService->Save();
        _localizationService->Reload();
    }

    void Close() { _romBrowserController->HideMenu(); }

private:
    IRomBrowserController* _romBrowserController;
    IAppSettingsService* _appSettingsService;
    ILocalizationService* _localizationService;
};
