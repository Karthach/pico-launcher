#pragma once
#include <memory>
#include <vector>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "FileAssociation.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 64> theme = "material";
    String<char, 256> lastUsedFilePath = "";
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
    // Keep a small bounded list so the settings file remains within its 8 KiB read limit.
    std::vector<String<char, 256>> favoritePaths;
};
