#pragma once
#include <memory>
#include "ILocalizationService.h"
#include "core/String.h"

#include "../settings/IAppSettingsService.h"

class JsonLocalizationService : public ILocalizationService
{
    struct StringEntry
    {
        String<char, 32> key;
        std::unique_ptr<char16_t[]> value;
    };

    StringEntry _strings[32];
    u32 _stringCount = 0;

public:
    explicit JsonLocalizationService(IAppSettingsService& appSettingsService);

    const char16_t* GetString(const char* key) const override;
};
