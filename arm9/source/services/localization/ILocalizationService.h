#pragma once

class ILocalizationService
{
public:
    virtual ~ILocalizationService() = default;

    virtual const char16_t* GetString(const char* key) const = 0;
};
