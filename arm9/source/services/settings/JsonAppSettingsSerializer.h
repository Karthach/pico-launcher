#pragma once
class AppSettings;

enum class SettingsLoadResult
{
    Loaded,
    Missing,
    Invalid
};

class JsonAppSettingsSerializer
{
public:
    void Serialize(const AppSettings* appSettings, const char* filePath) const;
    SettingsLoadResult Deserialize(AppSettings* appSettings, const char* filePath) const;
};
