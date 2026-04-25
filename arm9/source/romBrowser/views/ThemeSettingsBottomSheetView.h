#pragma once
#include "core/SharedPtr.h"
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/views/Label2DView.h"
#include "../viewModels/ThemeSettingsViewModel.h"

class ILocalizationService;

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

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    void SetGraphics(const ChipView::VramToken& chipViewVramToken);

private:
    ThemeSettingsViewModel* _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<ChipView> _themeChips[8];
    const MaterialColorScheme* _materialColorScheme;

    ThemeSettingsBottomSheetView(ThemeSettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        ILocalizationService& localizationService);
};
