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

    const char16_t* languageNames[] = { u"English", u"Español" };

    for (int i = 0; i < 2; i++)
    {
        _languageChips[i] = ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository);
        _languageChips[i]->SetText(languageNames[i]);
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
    BottomSheetView::Update();
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);

    const char* languages[] = { "english", "spanish" };
    const char* currentLang = _viewModel->GetCurrentLanguage();

    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->SetPosition(20 + (i % 2) * 110, _position.y + 46 + (i / 2) * 30);
        _languageChips[i]->SetSelected(strcmp(languages[i], currentLang) == 0);
    }
}

void LanguageSettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
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
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }

    if (inputProvider.Triggered(InputKey::A))
    {
        auto currentFocus = focusManager.GetCurrentFocus();
        const char* languages[] = { "english", "spanish" };
        for (int i = 0; i < 2; i++)
        {
            if (currentFocus.GetPointer() == _languageChips[i].GetPointer())
            {
                _viewModel->SetLanguage(languages[i]);
                _viewModel->Close();
                return true;
            }
        }
    }

    return false;
}

void LanguageSettingsBottomSheetView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenDown(touchPoint, focusManager);
    const char* languages[] = { "english", "spanish" };
    for (int i = 0; i < 2; i++)
    {
        if (_languageChips[i]->GetBounds().Contains(touchPoint))
        {
            focusManager.Focus(_languageChips[i]);
            _viewModel->SetLanguage(languages[i]);
            _viewModel->Close();
            break;
        }
    }
}

void LanguageSettingsBottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenUp(lastTouchPoint, focusManager);
}

void LanguageSettingsBottomSheetView::Focus(FocusManager& focusManager)
{
    const char* currentLang = _viewModel->GetCurrentLanguage();
    if (strcmp(currentLang, "spanish") == 0)
        focusManager.Focus(_languageChips[1]);
    else
        focusManager.Focus(_languageChips[0]);
}

void LanguageSettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

SharedPtr<View> LanguageSettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    int currentIdx = -1;
    for (int i = 0; i < 2; i++)
    {
        if (currentFocus.GetPointer() == _languageChips[i].GetPointer())
        {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx == -1) return nullptr;

    if (direction == FocusMoveDirection::Right && currentIdx == 0) return _languageChips[1];
    if (direction == FocusMoveDirection::Left && currentIdx == 1) return _languageChips[0];

    return nullptr;
}

void LanguageSettingsBottomSheetView::SetGraphics(const ChipView::VramToken& chipViewVramToken)
{
    for (int i = 0; i < 2; i++)
    {
        _languageChips[i]->SetGraphics(chipViewVramToken);
    }
}
