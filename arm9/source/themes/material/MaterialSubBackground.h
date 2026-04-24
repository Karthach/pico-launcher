#pragma once
#include "../background/IThemeBackground.h"
#include "MaterialColorScheme.h"

class MaterialSubBackground : public IThemeBackground
{
public:
    explicit MaterialSubBackground(const MaterialColorScheme* materialColorScheme)
        : _materialColorScheme(materialColorScheme), _frameCounter(0) { }

    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;
    void Update() override;
    void VBlank() override;

private:
    const MaterialColorScheme* _materialColorScheme;
    int _frameCounter;
};
