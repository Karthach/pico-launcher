#include "common.h"
#include <memory>
#include "json/ArduinoJson.h"
#include "AppSettings.h"
#include "fat/File.h"
#include "JsonAppSettingsSerializer.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  6144
#define MAX_SETTINGS_JSON_SIZE  8192

#define KEY_LANGUAGE                 "language"
#define KEY_ROM_BROWSER_LAYOUT       "romBrowserLayout"
#define KEY_ROM_BROWSER_SORT_MODE    "romBrowserSortMode"
#define KEY_THEME                    "theme"
#define KEY_LAST_USED_FILE_PATH      "lastUsedFilePath"
#define KEY_FILE_ASSOCIATIONS        "fileAssociations"
#define KEY_FILE_ASSOCIATIONS_APPLICATION_PATH  "appPath"
#define KEY_FAVORITES                "favorites"
#define MAX_FAVORITES                10

static const char* serializeRomBrowserLayout(RomBrowserLayout romBrowserLayout)
{
    switch (romBrowserLayout)
    {
        case RomBrowserLayout::HorizontalIconGrid:
            return "HorizontalIconGrid";
        case RomBrowserLayout::VerticalIconGrid:
            return "VerticalIconGrid";
        case RomBrowserLayout::BannerList:
            return "BannerList";
        case RomBrowserLayout::FileList:
            return "BannerList";
        case RomBrowserLayout::CoverFlow:
            return "CoverFlow";
        case RomBrowserLayout::InvertedCoverFlow:
            return "BannerList";
        default:
            return "";
    }
}

static bool tryParseRomBrowserLayout(
    const char* romBrowserLayoutString, RomBrowserLayout& romBrowserLayout)
{
    if (!romBrowserLayoutString)
        return false;

    if (!strcasecmp(romBrowserLayoutString, "HorizontalIconGrid"))
        romBrowserLayout = RomBrowserLayout::HorizontalIconGrid;
    else if (!strcasecmp(romBrowserLayoutString, "VerticalIconGrid"))
        romBrowserLayout = RomBrowserLayout::VerticalIconGrid;
    else if (!strcasecmp(romBrowserLayoutString, "BannerList"))
        romBrowserLayout = RomBrowserLayout::BannerList;
    else if (!strcasecmp(romBrowserLayoutString, "FileList"))
        romBrowserLayout = RomBrowserLayout::BannerList;
    else if (!strcasecmp(romBrowserLayoutString, "CoverFlow"))
        romBrowserLayout = RomBrowserLayout::CoverFlow;
    else if (!strcasecmp(romBrowserLayoutString, "InvertedCoverFlow"))
        romBrowserLayout = RomBrowserLayout::BannerList;
    else
        return false;

    return true;
}

static const char* serializeRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
{
    switch (romBrowserSortMode)
    {
        case RomBrowserSortMode::NameAscending:
            return "NameAscending";
        case RomBrowserSortMode::NameDescending:
            return "NameDescending";
        case RomBrowserSortMode::LastModified:
            return "LastModified";
        case RomBrowserSortMode::TitleAscending:
            return "TitleAscending";
        case RomBrowserSortMode::TitleDescending:
            return "TitleDescending";
        default:
            return "";
    }
}

static bool tryParseRomBrowserSortMode(
    const char* romBrowserDisplayModeString, RomBrowserSortMode& romBrowserSortMode)
{
    if (!romBrowserDisplayModeString)
        return false;

    if (!strcasecmp(romBrowserDisplayModeString, "NameAscending"))
        romBrowserSortMode = RomBrowserSortMode::NameAscending;
    else if (!strcasecmp(romBrowserDisplayModeString, "NameDescending"))
        romBrowserSortMode = RomBrowserSortMode::NameDescending;
    else if (!strcasecmp(romBrowserDisplayModeString, "LastModified"))
        romBrowserSortMode = RomBrowserSortMode::LastModified;
    else if (!strcasecmp(romBrowserDisplayModeString, "TitleAscending"))
        romBrowserSortMode = RomBrowserSortMode::TitleAscending;
    else if (!strcasecmp(romBrowserDisplayModeString, "TitleDescending"))
        romBrowserSortMode = RomBrowserSortMode::TitleDescending;
    else
        return false;

    return true;
}

static bool tryParseFileAssociations(const JsonObjectConst& json, AppSettings* appSettings)
{
    if (json.isNull())
    {
        return false;
    }

    appSettings->fileAssociations = std::make_unique_for_overwrite<FileAssociation[]>(json.size());
    int i = 0;
    for (auto item : json)
    {
        auto extension = item.key().c_str();
        auto appPath = item.value()[KEY_FILE_ASSOCIATIONS_APPLICATION_PATH].as<const char*>();
        appSettings->fileAssociations[i++] = FileAssociation(extension, appPath);
    }
    appSettings->numberOfFileAssociations = i;
    return true;
}

static void serializeFileAssociations(DynamicJsonDocument& json, const AppSettings* appSettings)
{
    auto jsonObject = json[KEY_FILE_ASSOCIATIONS].to<JsonObject>();
    for (u32 i = 0; i < appSettings->numberOfFileAssociations; i++)
    {
        const auto& fileAssociation = appSettings->fileAssociations[i];
        auto jsonAssociation = jsonObject[fileAssociation.extension.GetString()].to<JsonObject>();
        jsonAssociation[KEY_FILE_ASSOCIATIONS_APPLICATION_PATH] = fileAssociation.applicationPath.GetString();
    }
}

static void serializeFavorites(DynamicJsonDocument& json, const AppSettings* appSettings)
{
    auto favorites = json[KEY_FAVORITES].to<JsonArray>();
    const u32 count = std::min<u32>(appSettings->favoritePaths.size(), MAX_FAVORITES);
    for (u32 i = 0; i < count; i++)
        favorites.add(appSettings->favoritePaths[i].GetString());
}

static std::unique_ptr<u8[]> writeJson(const AppSettings* appSettings, u32& length)
{
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    json[KEY_LANGUAGE] = appSettings->language.GetString();
    json[KEY_ROM_BROWSER_LAYOUT] = serializeRomBrowserLayout(appSettings->romBrowserDisplaySettings.layout);
    json[KEY_ROM_BROWSER_SORT_MODE] = serializeRomBrowserSortMode(appSettings->romBrowserDisplaySettings.sortMode);
    json[KEY_THEME] = appSettings->theme.GetString();
    json[KEY_LAST_USED_FILE_PATH] = appSettings->lastUsedFilePath.GetString();
    serializeFileAssociations(json, appSettings);
    serializeFavorites(json, appSettings);

    u32 outputSize = measureJsonPretty(json);
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);

    serializeJsonPretty(json, fileData.get(), outputSize);

    length = outputSize;
    return fileData;
}

void JsonAppSettingsSerializer::Serialize(const AppSettings* appSettings, const char* filePath) const
{
    u32 length = 0;
    std::unique_ptr<u8[]> fileData = writeJson(appSettings, length);

    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open settings file for writing\n");
        return;
    }

    u32 bytesWritten;
    if (file->Write(fileData.get(), length, bytesWritten) != FR_OK || bytesWritten != length)
    {
        LOG_ERROR("Error while writing settings file\n");
        return;
    }

    LOG_DEBUG("Settings file written\n");
}

static void readJson(AppSettings* appSettings, const JsonDocument& json)
{
    appSettings->language = json[KEY_LANGUAGE] | appSettings->language.GetString();
    appSettings->theme = json[KEY_THEME] | appSettings->theme.GetString();
    appSettings->lastUsedFilePath = json[KEY_LAST_USED_FILE_PATH] | appSettings->lastUsedFilePath.GetString();

    RomBrowserLayout romBrowserLayout;
    if (tryParseRomBrowserLayout(json[KEY_ROM_BROWSER_LAYOUT].as<const char*>(),
            romBrowserLayout))
    {
        appSettings->romBrowserDisplaySettings.layout = romBrowserLayout;
    }
    RomBrowserSortMode romBrowserSortMode;
    if (tryParseRomBrowserSortMode(json[KEY_ROM_BROWSER_SORT_MODE].as<const char*>(),
            romBrowserSortMode))
    {
        appSettings->romBrowserDisplaySettings.sortMode = romBrowserSortMode;
    }

    tryParseFileAssociations(json[KEY_FILE_ASSOCIATIONS], appSettings);

    auto favorites = json[KEY_FAVORITES].as<JsonArrayConst>();
    appSettings->favoritePaths.clear();
    if (!favorites.isNull())
    {
        for (auto item : favorites)
        {
            const char* path = item.as<const char*>();
            if (!path || !path[0] || appSettings->favoritePaths.size() >= MAX_FAVORITES)
                continue;
            bool duplicate = false;
            for (const auto& favorite : appSettings->favoritePaths)
                duplicate |= !strcasecmp(favorite.GetString(), path);
            if (!duplicate)
                appSettings->favoritePaths.emplace_back(path);
        }
    }
}

SettingsLoadResult JsonAppSettingsSerializer::Deserialize(AppSettings* appSettings, const char* filePath) const
{
    const auto file = std::make_unique<File>();
    FRESULT openResult = file->Open(filePath, FA_READ | FA_OPEN_EXISTING);
    if (openResult != FR_OK)
    {
        return openResult == FR_NO_FILE || openResult == FR_NO_PATH
            ? SettingsLoadResult::Missing : SettingsLoadResult::Invalid;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0 || fileSize > MAX_SETTINGS_JSON_SIZE)
        return SettingsLoadResult::Invalid;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    if (!fileData || !file->ReadExact(fileData.get(), fileSize))
        return SettingsLoadResult::Invalid;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileData.get(), fileSize) != DeserializationError::Ok)
        return SettingsLoadResult::Invalid;

    readJson(appSettings, json);

    return SettingsLoadResult::Loaded;
}
