#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/views/Label2DView.h"
#include "services/localization/ILocalizationService.h"
#include "LanguageSettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

LanguageSettingsBottomSheetView::LanguageSettingsBottomSheetView(
    LanguageSettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, ILocalizationService& localizationService)
    : _viewModel(viewModel)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel->SetText(localizationService.GetString("language_settings_title"));
    AddChildTail(_titleLabel.GetPointer());

    const char* languages[] = { "english", "spanish" };
    const char16_t* languageNames[] = { u"English", u"Español" };

    for (int i = 0; i < 2; i++)
    {
        _languageChips[i] = ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository);
        _languageChips[i]->SetText(languageNames[i]);
        _languageChips[i]->SetSelected(strcmp(_viewModel->GetCurrentLanguage(), languages[i]) == 0);
        AddChildTail(_languageChips[i].GetPointer());
    }
}

void LanguageSettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->InitVram(vramContext);
    }
}

void LanguageSettingsBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->SetPosition(20 + (i % 2) * 110, _position.y + 40 + (i / 2) * 30);
    }
    BottomSheetView::Update();
}

void LanguageSettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    BottomSheetView::Draw(graphicsContext);
    _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
    _titleLabel->Draw(graphicsContext);
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->Draw(graphicsContext);
    }
}

void LanguageSettingsBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->VBlank();
    }
}

bool LanguageSettingsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        for (int i = 0; i < 2; i++)
        {
            if (focusManager.GetFocus() == _languageChips[i])
            {
                const char* languages[] = { "english", "spanish" };
                _viewModel->SetLanguage(languages[i]);
                _viewModel->Close();
                return true;
            }
        }
    }
    return BottomSheetView::HandleInput(inputProvider, focusManager);
}

void LanguageSettingsBottomSheetView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenDown(touchPoint, focusManager);
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->HandlePenDown(touchPoint, focusManager);
    }
}

void LanguageSettingsBottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenUp(lastTouchPoint, focusManager);
    for (int i = 0; i < 2; i++)
    {
        if (_languageChips[i]->HandlePenUp(lastTouchPoint, focusManager))
        {
            const char* languages[] = { "english", "spanish" };
            _viewModel->SetLanguage(languages[i]);
            _viewModel->Close();
        }
    }
}

void LanguageSettingsBottomSheetView::Focus(FocusManager& focusManager)
{
    for (int i = 0; i < 2; i++)
    {
        if (strcmp(_viewModel->GetCurrentLanguage(), (i == 0 ? "english" : "spanish")) == 0)
        {
            focusManager.Focus(_languageChips[i]);
            return;
        }
    }
    focusManager.Focus(_languageChips[0]);
}

void LanguageSettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

SharedPtr<View> LanguageSettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    for (int i = 0; i < 2; i++)
    {
        if (currentFocus == _languageChips[i])
        {
            if (direction == FocusMoveDirection::Right && i % 2 == 0 && i + 1 < 2) return _languageChips[i + 1];
            if (direction == FocusMoveDirection::Left && i % 2 == 1) return _languageChips[i - 1];
            if (direction == FocusMoveDirection::Down && i + 2 < 2) return _languageChips[i + 2];
            if (direction == FocusMoveDirection::Up && i - 2 >= 0) return _languageChips[i - 2];
        }
    }
    return nullptr;
}

void LanguageSettingsBottomSheetView::SetGraphics(const ChipView::VramToken& chipViewVramToken)
{
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->SetGraphics(chipViewVramToken);
    }
}
