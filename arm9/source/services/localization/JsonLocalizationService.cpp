#include "common.h"
#include <memory>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "core/StringUtil.h"
#include "JsonLocalizationService.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  2048

JsonLocalizationService::JsonLocalizationService(IAppSettingsService& appSettingsService)
{
    char langPath[128];
    snprintf(langPath, sizeof(langPath), "/_pico/lang/%s.json", appSettingsService.GetAppSettings().language.GetString());

    LOG_DEBUG("Loading language: %s\n", langPath);

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
    LOG_DEBUG("Lang file size: %d\n", fileSize);
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
    {
        LOG_ERROR("Read lang file fail\n");
        return;
    }

    LOG_DEBUG("Deserializing JSON...\n");
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    DeserializationError error = deserializeJson(json, fileDataPtr, fileSize);
    if (error != DeserializationError::Ok)
    {
        LOG_ERROR("JSON parse error: %s\n", error.c_str());
        return;
    }

    LOG_DEBUG("JSON parsed. Building map...\n");
    JsonObject root = json.as<JsonObject>();
    for (JsonPair it : root)
    {
        if (_stringCount >= 32) break;

        StringEntry& entry = _strings[_stringCount];
        entry.key = it.key().c_str();
        
        const char* val = it.value().as<const char*>();
        if (val)
        {
            size_t valLen = strlen(val);
            entry.value = std::make_unique<char16_t[]>(valLen + 1);
            StringUtil::Copy(entry.value.get(), val, valLen + 1);
        }
        _stringCount++;
    }
    
    LOG_DEBUG("Loaded %d strings\n", _stringCount);
}

const char16_t* JsonLocalizationService::GetString(const char* key) const
{
    for (u32 i = 0; i < _stringCount; i++)
    {
        if (!strcmp(_strings[i].key.GetString(), key))
        {
            return _strings[i].value.get();
        }
    }
    return u"";
}
