#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "listIcon.h"
#include "sortNameAscendingIcon.h"
#include "sortNameDescendingIcon.h"
#include "recentIcon.h"
#include "gamesIcon.h"
#include "picturesIcon.h"
#include "musicIcon.h"
#include "moviesIcon.h"
#include "unknownIcon.h"
#include "themeIcon.h"
#include "../IRomBrowserController.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "services/localization/ILocalizationService.h"
#include "DisplaySettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       8
#define THEME_BUTTON_X      212
#define THEME_BUTTON_Y      (TITLE_LABEL_Y - 7)

#define LAYOUT_LABEL_X      20
#define LAYOUT_LABEL_Y      34

#define LAYOUT_NAME_X       20
#define LAYOUT_NAME_Y       51

#define LAYOUT_CONTAINER_X      120
#define LAYOUT_CONTAINER_WIDTH  110
#define LAYOUT_CONTAINER_HEIGHT 32
#define LAYOUT_OPTIONS_Y        31

#define SORTING_LABEL_X     20
#define SORTING_LABEL_Y     70
#define SORTING_NAME_X      20
#define SORTING_NAME_Y      103
#define SORTING_OPTIONS_Y   69
#define SORTING_OPTIONS_X   120
#define SORTING_OPTIONS_WIDTH 110
#define SORTING_OPTIONS_HEIGHT 32

#define LANGUAGE_LABEL_X     20
#define LANGUAGE_LABEL_Y     135
#define LANGUAGE_OPTIONS_Y   132

static RomBrowserLayout sRomBrowserDisplayModes[] =
{
    RomBrowserLayout::HorizontalIconGrid,
    RomBrowserLayout::VerticalIconGrid,
    RomBrowserLayout::BannerList
};

static RomBrowserSortMode sRomBrowserSortModes[5] =
{
    [0] = RomBrowserSortMode::NameAscending,
    [1] = RomBrowserSortMode::NameDescending,
    [2] = RomBrowserSortMode::TitleAscending,
    [3] = RomBrowserSortMode::TitleDescending,
    [4] = RomBrowserSortMode::LastModified
};

DisplaySettingsBottomSheetView::DisplaySettingsBottomSheetView(
    DisplaySettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, ILocalizationService& localizationService)
    : _viewModel(viewModel)
    , _localizationService(localizationService)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _themeButton(IconButton2DView::CreateShared(IconButtonView::Type::Standard, IconButtonView::State::NoToggle, md::sys::color::inverseOnSurface, materialColorScheme))
    , _layoutLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _layoutNameLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium7_5)))
    , _sortingLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _sortingNameLabel(Label2DView::CreateShared(200, 16, 25, fontRepository->GetFont(FontType::Medium7_5)))
    , _languageLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
{
    _titleLabel->SetText(_localizationService.GetString("display_settings_title"));
    AddChildTail(_titleLabel.GetPointer());
    _themeButton->SetAction([] (IconButtonView*, void* arg)
    {
        ((DisplaySettingsBottomSheetView*)arg)->_viewModel->GotoSettingsScreen();
    }, this);
    AddChildTail(_themeButton.GetPointer());
    _layoutLabel->SetText(_localizationService.GetString("display_settings_layout"));
    AddChildTail(_layoutLabel.GetPointer());
    AddChildTail(_layoutNameLabel.GetPointer());
    _sortingLabel->SetText(_localizationService.GetString("display_settings_sorting"));
    AddChildTail(_sortingLabel.GetPointer());
    AddChildTail(_sortingNameLabel.GetPointer());
    _languageLabel->SetText(_localizationService.GetString("language_settings_title"));
    AddChildTail(_languageLabel.GetPointer());

    for (u32 i = 0; i < _layoutOptions.size(); i++)
    {
        _layoutOptions[i] = CreateLayoutOptionIconButton();
        _layoutOptions[i]->SetParent(this);
        // Do NOT add to child list, we draw manually with clipping
    }

    for (auto& sortOption : _sortOptions)
    {
        sortOption = CreateSortOptionIconButton();
        sortOption->SetParent(this);
        // Keep sort options outside the child list so drawing and touch input can be clipped to the scroll area.
    }

    const char16_t* languageNames[] = { u"English", u"Español" };
    for (u32 i = 0; i < _languageOptions.size(); i++)
    {
        _languageOptions[i] = CreateLanguageOptionChip();
        _languageOptions[i]->SetText(languageNames[i]);
        AddChildTail(_languageOptions[i].GetPointer());
    }
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateLayoutOptionIconButton()
{
    auto layoutOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    layoutOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_layoutOptions.size(); i++)
        {
            if (self->_layoutOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetRomBrowserDisplayMode(sRomBrowserDisplayModes[i]);
                break;
            }
        }
    }, this);
    return layoutOption;
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateSortOptionIconButton()
{
    auto sortOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    sortOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_sortOptions.size(); i++)
        {
            if (self->_sortOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetRomBrowserSortMode(sRomBrowserSortModes[i]);
                self->_isManualSortScroll = false;
                break;
            }
        }
    }, this);
    return sortOption;
}

SharedPtr<ChipView> DisplaySettingsBottomSheetView::CreateLanguageOptionChip()
{
    auto langOption = ChipView::CreateShared(
        md::sys::color::surfaceContainerLow,
        _materialColorScheme,
        _fontRepository
    );
    return langOption;
}

void DisplaySettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _themeButton->SetIconVramOffset(LoadIcon(*objVramManager, themeIconTiles, themeIconTilesLen));

        // layout options
        _layoutOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, hGridIconTiles, hGridIconTilesLen));
        _layoutOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, vGridIconTiles, vGridIconTilesLen));
        _layoutOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, bannerListIconTiles, bannerListIconTilesLen));

        // sort options
        _sortOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameAscendingIconTiles, sortNameAscendingIconTilesLen));
        _sortOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameDescendingIconTiles, sortNameDescendingIconTilesLen));
        _sortOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, gamesIconTiles, gamesIconTilesLen));
        _sortOptions[3]->SetIconVramOffset(LoadIcon(*objVramManager, gamesIconTiles, gamesIconTilesLen));
        _sortOptions[4]->SetIconVramOffset(LoadIcon(*objVramManager, recentIconTiles, recentIconTilesLen));
    }

    _layoutNameLabel->InitVram(vramContext);

    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->InitVram(vramContext);
    }

    for (auto& sortOption : _sortOptions)
    {
        sortOption->InitVram(vramContext);
    }

    for (auto& langOption : _languageOptions)
    {
        langOption->InitVram(vramContext);
    }
}

void DisplaySettingsBottomSheetView::UpdateLabels()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _themeButton->SetPosition(THEME_BUTTON_X, _position.y + THEME_BUTTON_Y);
    _layoutLabel->SetPosition(LAYOUT_LABEL_X, _position.y + LAYOUT_LABEL_Y);
    _layoutNameLabel->SetPosition(LAYOUT_NAME_X, _position.y + LAYOUT_NAME_Y);
    _sortingLabel->SetPosition(SORTING_LABEL_X, _position.y + SORTING_LABEL_Y);
    _sortingNameLabel->SetPosition(SORTING_NAME_X, _position.y + SORTING_NAME_Y);
    _languageLabel->SetPosition(LANGUAGE_LABEL_X, _position.y + LANGUAGE_LABEL_Y);
}

void DisplaySettingsBottomSheetView::ClampSortScroll()
{
    const int minScrollX = SORTING_OPTIONS_WIDTH - (int)_sortOptions.size() * 32;
    if (_sortScrollX > 0) _sortScrollX = 0;
    if (_sortScrollX < minScrollX) _sortScrollX = minScrollX;
}

void DisplaySettingsBottomSheetView::EnsureSortOptionVisible(u32 index)
{
    const int optionX = 120 + _sortScrollX + (int)index * 32;
    if (optionX < 120)
        _sortScrollX += 120 - optionX;
    else if (optionX + 32 > 120 + SORTING_OPTIONS_WIDTH)
        _sortScrollX -= optionX + 32 - (120 + SORTING_OPTIONS_WIDTH);
    ClampSortScroll();
    _isManualSortScroll = true;
}

void DisplaySettingsBottomSheetView::Update()
{
    BottomSheetView::Update();
    UpdateLabels();
    auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();

    const char* layoutKeys[] = {
        "layout_horizontal_icon_grid",
        "layout_vertical_icon_grid",
        "layout_banner_list"
    };

    int layoutIdx = 0;
    for (u32 i = 0; i < (u32)_layoutOptions.size(); i++)
    {
        if (sRomBrowserDisplayModes[i] == selectedDisplayMode)
        {
            layoutIdx = i;
            break;
        }
    }
    
    _layoutNameLabel->SetText(_localizationService.GetString(layoutKeys[layoutIdx]));

    const char* sortKeys[] = {
        "sort_name_ascending",
        "sort_name_descending",
        "sort_title_ascending",
        "sort_title_descending",
        "sort_last_modified"
    };
    int sortIdx = 0;
    for (u32 i = 0; i < (u32)_sortOptions.size(); i++)
    {
        if (sRomBrowserSortModes[i] == _viewModel->GetRomBrowserSortMode())
        {
            sortIdx = i;
            break;
        }
    }
    _sortingNameLabel->SetText(_localizationService.GetString(sortKeys[sortIdx]));

    if (!_isManualSortScroll)
    {
        _sortScrollX = -(sortIdx * 32);
        ClampSortScroll();
    }

    int x = 0;
    u32 idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetPosition(LAYOUT_CONTAINER_X + x, _position.y + LAYOUT_OPTIONS_Y);
        layoutOption->SetState(sRomBrowserDisplayModes[idx] == selectedDisplayMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        layoutOption->Update();
        x += 32;
        idx++;
    }

    auto selectedSortMode = _viewModel->GetRomBrowserSortMode();
    x = SORTING_OPTIONS_X + _sortScrollX;
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetPosition(x, _position.y + SORTING_OPTIONS_Y);
        sortOption->SetState(sRomBrowserSortModes[idx] == selectedSortMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        sortOption->Update();
        x += 32;
        idx++;
    }

    auto currentLang = _viewModel->GetLanguage();
    x = 120;
    idx = 0;
    const char* languages[] = { "english", "spanish" };
    for (auto& langOption : _languageOptions)
    {
        langOption->SetPosition(x, _position.y + LANGUAGE_OPTIONS_Y);
        langOption->SetSelected(strcmp(languages[idx], currentLang) == 0);
        x += langOption->GetWidth() + 8;
        idx++;
    }
}

void DisplaySettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _layoutLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _layoutLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _layoutNameLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _layoutNameLabel->SetForegroundColor(_materialColorScheme->primary);
        _sortingLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _sortingLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _sortingNameLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _sortingNameLabel->SetForegroundColor(_materialColorScheme->primary);
        _languageLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _languageLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        
        BottomSheetView::Draw(graphicsContext);

        // Clip layout options
        Rectangle layoutClip(LAYOUT_CONTAINER_X, _position.y + LAYOUT_OPTIONS_Y, LAYOUT_CONTAINER_WIDTH, LAYOUT_CONTAINER_HEIGHT);
        graphicsContext.SetClipArea(layoutClip);
        for (auto& layoutOption : _layoutOptions)
        {
            layoutOption->Draw(graphicsContext);
        }
        graphicsContext.SetClipArea(GetBounds());

        Rectangle sortClip(SORTING_OPTIONS_X, _position.y + SORTING_OPTIONS_Y,
            SORTING_OPTIONS_WIDTH, SORTING_OPTIONS_HEIGHT);
        graphicsContext.SetClipArea(sortClip);
        for (auto& sortOption : _sortOptions)
        {
            sortOption->Draw(graphicsContext);
        }
        graphicsContext.SetClipArea(GetBounds());

        for (auto& langOption : _languageOptions)
        {
            langOption->Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void DisplaySettingsBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->VBlank();
    }
    for (auto& sortOption : _sortOptions)
    {
        sortOption->VBlank();
    }
    for (auto& langOption : _languageOptions)
    {
        langOption->VBlank();
    }
}

bool DisplaySettingsBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    if (inputProvider.Triggered(InputKey::A))
    {
        auto currentFocus = focusManager.GetCurrentFocus();
        if (currentFocus.GetPointer() == _themeButton.GetPointer())
        {
            _themeButton->HandleInput(inputProvider, focusManager);
            return true;
        }
        for (auto& layoutOption : _layoutOptions)
        {
            if (currentFocus.GetPointer() == layoutOption.GetPointer())
            {
                layoutOption->HandleInput(inputProvider, focusManager);
                return true;
            }
        }
        const char* languages[] = { "english", "spanish" };
        for (u32 i = 0; i < _languageOptions.size(); i++)
        {
            if (currentFocus.GetPointer() == _languageOptions[i].GetPointer())
            {
                _viewModel->SetLanguage(languages[i]);
                return true;
            }
        }
    }
    return false;
}

void DisplaySettingsBottomSheetView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenDown(touchPoint, focusManager);
    
    Rectangle layoutRect(LAYOUT_CONTAINER_X, _position.y + LAYOUT_OPTIONS_Y, LAYOUT_CONTAINER_WIDTH, LAYOUT_CONTAINER_HEIGHT);
    for (auto& layoutOption : _layoutOptions)
    {
        // Adjust touch check for clipped area
        if (layoutRect.Contains(touchPoint) && layoutOption->GetBounds().Contains(touchPoint))
        {
            layoutOption->HandlePenDown(touchPoint, focusManager);
        }
    }

    Rectangle sortRect(SORTING_OPTIONS_X, _position.y + SORTING_OPTIONS_Y,
        SORTING_OPTIONS_WIDTH, SORTING_OPTIONS_HEIGHT);
    if (sortRect.Contains(touchPoint))
    {
        _isDraggingSort = true;
        _isManualSortScroll = true;
        _lastSortTouchPoint = touchPoint;
        for (auto& sortOption : _sortOptions)
        {
            if (sortOption->GetBounds().Contains(touchPoint))
            {
                sortOption->HandlePenDown(touchPoint, focusManager);
            }
        }
    }

    const char* languages[] = { "english", "spanish" };
    for (u32 i = 0; i < _languageOptions.size(); i++)
    {
        if (_languageOptions[i]->GetBounds().Contains(touchPoint))
        {
            focusManager.Focus(_languageOptions[i]);
            _viewModel->SetLanguage(languages[i]);
            break;
        }
    }
}

void DisplaySettingsBottomSheetView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenMove(touchPoint, focusManager);
    if (_isDraggingSort)
    {
        _sortScrollX += touchPoint.x - _lastSortTouchPoint.x;
        ClampSortScroll();
        _lastSortTouchPoint = touchPoint;
        for (auto& sortOption : _sortOptions)
        {
            sortOption->HandlePenMove(touchPoint, focusManager);
        }
    }
}

void DisplaySettingsBottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenUp(lastTouchPoint, focusManager);
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->HandlePenUp(lastTouchPoint, focusManager);
    }
    for (auto& sortOption : _sortOptions)
    {
        sortOption->HandlePenUp(lastTouchPoint, focusManager);
    }
    _isDraggingSort = false;
}

SharedPtr<View> DisplaySettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (currentFocus.GetPointer() == _themeButton.GetPointer())
    {
        return direction == FocusMoveDirection::Down ? _layoutOptions[0] : nullptr;
    }

    int idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        if (currentFocus.GetPointer() == layoutOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0) idx = 0;
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_layoutOptions.size()) idx = _layoutOptions.size() - 1;
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                return _themeButton;
            }
            else if (direction == FocusMoveDirection::Down)
            {
                auto selectedSortMode = _viewModel->GetRomBrowserSortMode();
                for (u32 i = 0; i < (u32)_sortOptions.size(); i++)
                {
                    if (sRomBrowserSortModes[i] == selectedSortMode)
                    {
                        EnsureSortOptionVisible(i);
                        return _sortOptions[i];
                    }
                }
                EnsureSortOptionVisible(0);
                return _sortOptions[0];
            }
        }
        idx++;
    }
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        if (currentFocus.GetPointer() == sortOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                idx = idx == 0 ? (int)_sortOptions.size() - 1 : idx - 1;
                EnsureSortOptionVisible((u32)idx);
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                idx = idx + 1 >= (int)_sortOptions.size() ? 0 : idx + 1;
                EnsureSortOptionVisible((u32)idx);
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();
                for (u32 i = 0; i < (u32)_layoutOptions.size(); i++)
                {
                    if (sRomBrowserDisplayModes[i] == selectedDisplayMode)
                    {
                        return _layoutOptions[i];
                    }
                }
                return _layoutOptions[0];
            }
            else if (direction == FocusMoveDirection::Down)
            {
                return _languageOptions[0];
            }
        }
        idx++;
    }
    idx = 0;
    for (auto& langOption : _languageOptions)
    {
        if (currentFocus.GetPointer() == langOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _languageOptions.size();
                return _languageOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_languageOptions.size())
                    idx = 0;
                return _languageOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                return _sortOptions[0];
            }
        }
        idx++;
    }
    return nullptr;
}

void DisplaySettingsBottomSheetView::SetGraphics(
    const IconButton2DView::VramToken& iconButtonVramToken,
    const ChipView::VramToken& chipViewVramToken)
{
    _themeButton->SetGraphics(iconButtonVramToken);
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& langOption : _languageOptions)
    {
        langOption->SetGraphics(chipViewVramToken);
    }
}

void DisplaySettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

void DisplaySettingsBottomSheetView::Focus(FocusManager& focusManager)
{
    auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();
    for (u32 i = 0; i < (u32)_layoutOptions.size(); i++)
    {
        if (sRomBrowserDisplayModes[i] == selectedDisplayMode)
        {
            focusManager.Focus(_layoutOptions[i]);
            return;
        }
    }
    focusManager.Focus(_layoutOptions[0]);
}

u32 DisplaySettingsBottomSheetView::LoadIcon(IVramManager& vramManager,
    const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}
