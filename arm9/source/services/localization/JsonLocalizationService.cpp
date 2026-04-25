#include "common.h"
#include <memory>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "core/StringUtil.h"
#include "JsonLocalizationService.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  4096

JsonLocalizationService::JsonLocalizationService(IAppSettingsService& appSettingsService)
{
    char langPath[128];
    snprintf(langPath, sizeof(langPath), "/_pico/lang/%s.json", appSettingsService.GetAppSettings().language.GetString());

    const auto file = std::make_unique<File>();
    if (file->Open(langPath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        LOG_DEBUG("Language file %s not found. Trying english.json\n", langPath);
        if (file->Open("/_pico/lang/english.json", FA_READ | FA_OPEN_EXISTING) != FR_OK)
        {
            LOG_ERROR("Couldn't open any language file\n");
            return;
        }
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
    {
        LOG_ERROR("Failed to parse language file %s\n", langPath);
        return;
    }

    JsonObject root = json.as<JsonObject>();
    for (JsonPair it : root)
    {
        StringEntry entry;
        entry.key = it.key().c_str();
        
        const char* val = it.value().as<const char*>();
        if (val)
        {
            size_t valLen = strlen(val);
            entry.value = std::make_unique<char16_t[]>(valLen + 1);
            StringUtil::Copy(entry.value.get(), val, valLen + 1);
        }
        _strings.push_back(std::move(entry));
    }
    
    LOG_DEBUG("Loaded %d strings\n", _strings.size());
}

const char16_t* JsonLocalizationService::GetString(const char* key) const
{
    for (const auto& entry : _strings)
    {
        if (entry.key == key)
        {
            return entry.value.get();
        }
    }
    return u"";
}
