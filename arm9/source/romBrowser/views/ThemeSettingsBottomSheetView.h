#pragma once
#include "BottomSheetView.h"
#include "ChipView.h"
#include "../viewModels/ThemeSettingsViewModel.h"

class ThemeSettingsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(ThemeSettingsBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override;
    void Close() override;

    void SetGraphics(const ChipView::VramToken& chipViewVramToken);

private:
    ThemeSettingsViewModel* _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<ChipView> _themeChips[8];
    const MaterialColorScheme* _materialColorScheme;

    ThemeSettingsBottomSheetView(ThemeSettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);
};
