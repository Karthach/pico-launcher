#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "ThemeSettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

ThemeSettingsBottomSheetView::ThemeSettingsBottomSheetView(
    ThemeSettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _viewModel(viewModel)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel->SetText(u"Theme Settings");
    AddChildTail(_titleLabel.GetPointer());

    for (int i = 0; i < 8; i++)
    {
        _themeChips[i] = ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository);
        if (i < _viewModel->GetThemeCount())
        {
            const char* themeName = _viewModel->GetTheme(i);
            char16_t themeName16[64];
            for (int j = 0; j < 64; j++)
            {
                themeName16[j] = (char16_t)themeName[j];
                if (themeName[j] == 0) break;
            }
            _themeChips[i]->SetText(themeName16);
            AddChildTail(_themeChips[i].GetPointer());
        }
    }
}

void ThemeSettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);
    for (int i = 0; i < _viewModel->GetThemeCount(); i++)
    {
        _themeChips[i]->InitVram(vramContext);
    }
}

void ThemeSettingsBottomSheetView::Update()
{
    BottomSheetView::Update();
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);

    int x = 20;
    int y = _position.y + 46;
    const char* currentTheme = _viewModel->GetCurrentTheme();

    for (int i = 0; i < _viewModel->GetThemeCount(); i++)
    {
        _themeChips[i]->SetPosition(x, y);
        _themeChips[i]->SetSelected(strcmp(_viewModel->GetTheme(i), currentTheme) == 0);
        
        x += _themeChips[i]->GetWidth() + 8;
        if (x > 210)
        {
            x = 20;
            y += 28;
        }
    }
}

void ThemeSettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
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

void ThemeSettingsBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
    for (int i = 0; i < _viewModel->GetThemeCount(); i++)
    {
        _themeChips[i]->VBlank();
    }
}

bool ThemeSettingsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void ThemeSettingsBottomSheetView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenDown(touchPoint, focusManager);
    for (int i = 0; i < _viewModel->GetThemeCount(); i++)
    {
        if (_themeChips[i]->GetBounds().Contains(touchPoint))
        {
            _viewModel->SetTheme(_viewModel->GetTheme(i));
            break;
        }
    }
}

void ThemeSettingsBottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenUp(lastTouchPoint, focusManager);
}

void ThemeSettingsBottomSheetView::Focus(FocusManager& focusManager)
{
    if (_viewModel->GetThemeCount() > 0)
        focusManager.Focus(_themeChips[0]);
}

void ThemeSettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

void ThemeSettingsBottomSheetView::SetGraphics(const ChipView::VramToken& chipViewVramToken)
{
    for (int i = 0; i < 8; i++)
    {
        _themeChips[i]->SetGraphics(chipViewVramToken);
    }
}
