#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "services/localization/ILocalizationService.h"
#include "ThemeSettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

ThemeSettingsBottomSheetView::ThemeSettingsBottomSheetView(
    ThemeSettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, ILocalizationService& localizationService)
    : _viewModel(viewModel)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _materialColorScheme(materialColorScheme)
{
    _viewModel->RefreshThemes();
    _titleLabel->SetText(localizationService.GetString("theme_settings_title"));
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

    if (inputProvider.Triggered(InputKey::A))
    {
        auto currentFocus = focusManager.GetCurrentFocus();
        for (int i = 0; i < _viewModel->GetThemeCount(); i++)
        {
            if (currentFocus.GetPointer() == _themeChips[i].GetPointer())
            {
                _viewModel->SetTheme(_viewModel->GetTheme(i));
                return true;
            }
        }
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
            focusManager.Focus(_themeChips[i]);
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
    {
        const char* currentTheme = _viewModel->GetCurrentTheme();
        int currentIdx = 0;
        for (int i = 0; i < _viewModel->GetThemeCount(); i++)
        {
            if (strcmp(_viewModel->GetTheme(i), currentTheme) == 0)
            {
                currentIdx = i;
                break;
            }
        }
        focusManager.Focus(_themeChips[currentIdx]);
    }
}

void ThemeSettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

SharedPtr<View> ThemeSettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    int currentIdx = -1;
    for (int i = 0; i < _viewModel->GetThemeCount(); i++)
    {
        if (currentFocus.GetPointer() == _themeChips[i].GetPointer())
        {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx == -1) return nullptr;

    Point currentCenter = _themeChips[currentIdx]->GetBounds().GetCenter();
    int bestIdx = -1;
    s64 bestScore = 0x7FFFFFFFFFFFFFFFLL;

    for (int i = 0; i < _viewModel->GetThemeCount(); i++)
    {
        if (i == currentIdx) continue;

        Point targetCenter = _themeChips[i]->GetBounds().GetCenter();
        int dx = targetCenter.x - currentCenter.x;
        int dy = targetCenter.y - currentCenter.y;

        bool isCorrectDirection = false;
        s64 score = 0;

        switch (direction)
        {
            case FocusMoveDirection::Up:
                if (dy < -10) {
                    isCorrectDirection = true;
                    score = (s64)(-dy) * 100 + std::abs(dx);
                }
                break;
            case FocusMoveDirection::Down:
                if (dy > 10) {
                    isCorrectDirection = true;
                    score = (s64)dy * 100 + std::abs(dx);
                }
                break;
            case FocusMoveDirection::Left:
                if (dx < 0 && std::abs(dy) < 15) {
                    isCorrectDirection = true;
                    score = (s64)(-dx) + std::abs(dy) * 100;
                }
                break;
            case FocusMoveDirection::Right:
                if (dx > 0 && std::abs(dy) < 15) {
                    isCorrectDirection = true;
                    score = (s64)dx + std::abs(dy) * 100;
                }
                break;
        }

        if (isCorrectDirection && score < bestScore)
        {
            bestScore = score;
            bestIdx = i;
        }
    }

    if (bestIdx != -1) return _themeChips[bestIdx];

    // Navegación circular de respaldo para Izquierda/Derecha
    if (direction == FocusMoveDirection::Left) {
        int idx = currentIdx - 1;
        if (idx < 0) idx = _viewModel->GetThemeCount() - 1;
        return _themeChips[idx];
    }
    if (direction == FocusMoveDirection::Right) {
        int idx = currentIdx + 1;
        if (idx >= _viewModel->GetThemeCount()) idx = 0;
        return _themeChips[idx];
    }

    return nullptr;
}

void ThemeSettingsBottomSheetView::SetGraphics(const ChipView::VramToken& chipViewVramToken)
{
    for (int i = 0; i < 8; i++)
    {
        _themeChips[i]->SetGraphics(chipViewVramToken);
    }
}
