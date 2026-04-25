#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "ILocalizationService.h"
#include "core/String.h"

#include "../settings/IAppSettingsService.h"

class JsonLocalizationService : public ILocalizationService
{
    struct StringEntry
    {
        std::string key;
        std::unique_ptr<char16_t[]> value;
    };

    std::vector<StringEntry> _strings;

public:
    explicit JsonLocalizationService(IAppSettingsService& appSettingsService);

    const char16_t* GetString(const char* key) const override;
};
