#include "common.h"
#include "JsonAppSettingsService.h"

JsonAppSettingsService::JsonAppSettingsService(const char* filePath)
    : _filePath(filePath)
{
    auto loadResult = _serializer.Deserialize(&_appSettings, _filePath);
    if (loadResult != SettingsLoadResult::Loaded)
    {
        if (loadResult == SettingsLoadResult::Invalid)
            LOG_ERROR("Settings file is invalid or too large; restoring defaults.\n");
        Save();
    }
}
