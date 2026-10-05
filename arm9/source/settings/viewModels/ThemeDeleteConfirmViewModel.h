#pragma once
#include "core/mini-printf.h"
#include "core/String.h"
#include "fat/ff.h"
#include "romBrowser/viewModels/IDeleteConfirmViewModel.h"
#include "settings/ISettingsController.h"
#include "services/localization/ILocalizationService.h"
#include <string.h>

/// @brief The delete sheet for a theme. The list shows theme names, and two
///        themes can share one, so the sheet names the folder too. Copies its
///        texts when it is built, so nothing dangles while it animates.
class ThemeDeleteConfirmViewModel : public IDeleteConfirmViewModel
{
public:
    ThemeDeleteConfirmViewModel(ISettingsController* settingsController, ILocalizationService* localizationService)
        : _settingsController(settingsController)
        , _localizationService(localizationService)
        , _themeName(settingsController->GetDeleteThemeName())
        , _folderName(settingsController->GetDeleteFolderName())
    {
        // display only: a cut-off folder name here ends in an ellipsis on screen
        mini_snprintf(_detailLine, sizeof(_detailLine), "%s", _folderName.GetString());
    }

    const char16_t* GetTitle() const override { return u"Delete theme?"; }

    const char16_t* GetNameLine16() const override
    {
        // a theme without a name, or a folder with no readable theme.json,
        // is named by its folder instead
        return _themeName.GetString()[0] != 0 ? _themeName.GetString() : nullptr;
    }

    const char* GetNameLine() const override { return _folderName.GetString(); }
    const char* GetDetailLine() const override { return _detailLine; }
    const char* GetDetailKey() const override { return "delete_theme_folder_detail"; }
    const char* GetDetailArgument() const override { return _detailLine; }
    const char* GetStatusLine() const override { return _settingsController->GetDeleteStatus(); }
    const char* GetStatusKey() const override
    {
        const char* status = _settingsController->GetDeleteStatus();
        if (!status) return nullptr;
        if (strcmp(status, "Deleted") == 0) return "delete_status_ok";
        if (strcmp(status, "Couldn't find that theme's folder") == 0) return "delete_status_not_found";
        if (strcmp(status, "That theme can't be deleted") == 0) return "delete_status_protected";
        if (strcmp(status, "Couldn't delete: a file is read-only") == 0) return "delete_status_read_only";
        if (strcmp(status, "Couldn't delete: too many folders deep") == 0) return "delete_status_too_deep";
        if (strcmp(status, "Couldn't delete: too many files") == 0) return "delete_status_too_many_files";
        if (strcmp(status, "Couldn't delete: a name is too long") == 0) return "delete_status_name_long";
        if (strcmp(status, "Couldn't delete: a name can't be read") == 0) return "delete_status_bad_name";
        if (strcmp(status, "Couldn't read the card") == 0) return "delete_status_read_error";
        if (strcmp(status, "Couldn't write to the card") == 0) return "delete_status_write_error";
        if (strcmp(status, "Not enough memory, try again") == 0) return "delete_status_memory";
        if (strcmp(status, "Couldn't delete it all, try again") == 0) return "delete_status_partial";
        return nullptr;
    }

    void Confirm() override { _settingsController->ConfirmDeleteTheme(); }
    void Cancel() override { _settingsController->CancelDeleteTheme(); }

private:
    ISettingsController* _settingsController;
    ILocalizationService* _localizationService;
    String<char16_t, 65> _themeName;
    String<char, FF_LFN_BUF + 1> _folderName;
    char _detailLine[FF_LFN_BUF + 64];
};
