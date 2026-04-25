#pragma once
#include "core/SharedPtr.h"
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/views/Label2DView.h"
#include "../viewModels/LanguageSettingsViewModel.h"

class ILocalizationService;

class LanguageSettingsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(LanguageSettingsBottomSheetView)

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
    LanguageSettingsViewModel* _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<ChipView> _languageChips[2];
    const MaterialColorScheme* _materialColorScheme;

    LanguageSettingsBottomSheetView(LanguageSettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        ILocalizationService& localizationService);
};
