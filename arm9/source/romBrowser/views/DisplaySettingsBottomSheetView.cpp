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
#include "coverflowIcon.h"
#include "upIcon.h"
#include "../IRomBrowserController.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "services/localization/ILocalizationService.h"
#include "DisplaySettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LAYOUT_LABEL_X      20
#define LAYOUT_LABEL_Y      38

#define LAYOUT_NAME_X       20
#define LAYOUT_NAME_Y       52

#define LAYOUT_CONTAINER_X      120
#define LAYOUT_CONTAINER_WIDTH  110
#define LAYOUT_CONTAINER_HEIGHT 32

#define SORTING_LABEL_X     20
#define SORTING_LABEL_Y     82

#define LANGUAGE_LABEL_X     20
#define LANGUAGE_LABEL_Y     120

static RomBrowserLayout sRomBrowserDisplayModes[] =
{
    RomBrowserLayout::HorizontalIconGrid,
    RomBrowserLayout::VerticalIconGrid,
    RomBrowserLayout::BannerList,
    RomBrowserLayout::CoverFlow,
    RomBrowserLayout::InvertedCoverFlow
};

static RomBrowserSortMode sRomBrowserSortModes[3] =
{
    [0] = RomBrowserSortMode::NameAscending,
    [1] = RomBrowserSortMode::NameDescending,
    [2] = RomBrowserSortMode::TitleAscending
};

DisplaySettingsBottomSheetView::DisplaySettingsBottomSheetView(
    DisplaySettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, ILocalizationService& localizationService)
    : _viewModel(viewModel)
    , _localizationService(localizationService)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _layoutLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _layoutNameLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium7_5)))
    , _sortingLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _languageLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _layoutScrollX(0)
    , _isDraggingLayout(false)
    , _isManualScroll(false)
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
{
    _titleLabel->SetText(_localizationService.GetString("display_settings_title"));
    AddChildTail(_titleLabel.GetPointer());
    _layoutLabel->SetText(_localizationService.GetString("display_settings_layout"));
    AddChildTail(_layoutLabel.GetPointer());
    AddChildTail(_layoutNameLabel.GetPointer());
    _sortingLabel->SetText(_localizationService.GetString("display_settings_sorting"));
    AddChildTail(_sortingLabel.GetPointer());
    _languageLabel->SetText(_localizationService.GetString("language_settings_title"));
    AddChildTail(_languageLabel.GetPointer());

    for (u32 i = 0; i < _layoutOptions.size(); i++)
    {
        _layoutOptions[i] = CreateLayoutOptionIconButton();
        // Do NOT add to child list, we draw manually with clipping
    }

    for (auto& sortOption : _sortOptions)
    {
        sortOption = CreateSortOptionIconButton();
        AddChildTail(sortOption.GetPointer());
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
                self->_isManualScroll = false; // Center on click
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
        // layout options
        _layoutOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, hGridIconTiles, hGridIconTilesLen));
        _layoutOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, vGridIconTiles, vGridIconTilesLen));
        _layoutOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, bannerListIconTiles, bannerListIconTilesLen));
        _layoutOptions[3]->SetIconVramOffset(LoadIcon(*objVramManager, coverflowIconTiles, coverflowIconTilesLen));
        _layoutOptions[4]->SetIconVramOffset(LoadIcon(*objVramManager, upIconTiles, upIconTilesLen)); // Using upIcon as placeholder

        // sort options
        _sortOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameAscendingIconTiles, sortNameAscendingIconTilesLen));
        _sortOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameDescendingIconTiles, sortNameDescendingIconTilesLen));
        _sortOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, gamesIconTiles, gamesIconTilesLen));
    }

    _layoutNameLabel->InitVram(vramContext);

    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->InitVram(vramContext);
    }

    for (auto& langOption : _languageOptions)
    {
        langOption->InitVram(vramContext);
    }
}

void DisplaySettingsBottomSheetView::UpdateLabels()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _layoutLabel->SetPosition(LAYOUT_LABEL_X, _position.y + LAYOUT_LABEL_Y);
    _layoutNameLabel->SetPosition(LAYOUT_NAME_X, _position.y + LAYOUT_NAME_Y);
    _sortingLabel->SetPosition(SORTING_LABEL_X, _position.y + SORTING_LABEL_Y);
    _languageLabel->SetPosition(LANGUAGE_LABEL_X, _position.y + LANGUAGE_LABEL_Y);
}

void DisplaySettingsBottomSheetView::Update()
{
    BottomSheetView::Update();
    UpdateLabels();
    auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();

    const char* layoutKeys[] = {
        "layout_horizontal_icon_grid",
        "layout_vertical_icon_grid",
        "layout_banner_list",
        "layout_cover_flow",
        "layout_inverted_cover_flow"
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

    // Auto-scroll logic
    if (!_isManualScroll)
    {
        int targetX = -(layoutIdx * 32);
        int maxX = 0;
        int minX = LAYOUT_CONTAINER_WIDTH - ((int)_layoutOptions.size() * 32);
        if (targetX > maxX) targetX = maxX;
        if (targetX < minX) targetX = minX;
        
        _layoutScrollX = targetX; 
    }

    int x = 0;
    u32 idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetPosition(LAYOUT_CONTAINER_X + _layoutScrollX + x, _position.y + (LAYOUT_LABEL_Y - 3));
        layoutOption->SetState(sRomBrowserDisplayModes[idx] == selectedDisplayMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        layoutOption->Update();
        x += 32;
        idx++;
    }

    auto selectedSortMode = _viewModel->GetRomBrowserSortMode();
    x = 120;
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetPosition(x, _position.y + (SORTING_LABEL_Y - 8));
        sortOption->SetState(sRomBrowserSortModes[idx] == selectedSortMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        x += 32;
        idx++;
    }

    auto currentLang = _viewModel->GetLanguage();
    x = 120;
    idx = 0;
    const char* languages[] = { "english", "spanish" };
    for (auto& langOption : _languageOptions)
    {
        langOption->SetPosition(x, _position.y + (LANGUAGE_LABEL_Y - 3));
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
        _languageLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _languageLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        
        BottomSheetView::Draw(graphicsContext);

        // Clip layout options
        Rectangle layoutClip(LAYOUT_CONTAINER_X, _position.y + (LAYOUT_LABEL_Y - 8), LAYOUT_CONTAINER_WIDTH, LAYOUT_CONTAINER_HEIGHT);
        graphicsContext.SetClipArea(layoutClip);
        for (auto& layoutOption : _layoutOptions)
        {
            layoutOption->Draw(graphicsContext);
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
    
    Rectangle layoutRect(LAYOUT_CONTAINER_X, _position.y + (LAYOUT_LABEL_Y - 8), LAYOUT_CONTAINER_WIDTH, LAYOUT_CONTAINER_HEIGHT);
    if (layoutRect.Contains(touchPoint))
    {
        _isDraggingLayout = true;
        _isManualScroll = true;
        _lastTouchPoint = touchPoint;
    }

    for (auto& layoutOption : _layoutOptions)
    {
        // Adjust touch check for clipped area
        if (layoutRect.Contains(touchPoint) && layoutOption->GetBounds().Contains(touchPoint))
        {
            layoutOption->HandlePenDown(touchPoint, focusManager);
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
    if (_isDraggingLayout)
    {
        int dx = touchPoint.x - _lastTouchPoint.x;
        _layoutScrollX += dx;
        int maxX = 0;
        int minX = LAYOUT_CONTAINER_WIDTH - ((int)_layoutOptions.size() * 32);
        if (_layoutScrollX > maxX) _layoutScrollX = maxX;
        if (_layoutScrollX < minX) _layoutScrollX = minX;
        _lastTouchPoint = touchPoint;
    }
}

void DisplaySettingsBottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    BottomSheetView::HandlePenUp(lastTouchPoint, focusManager);
    _isDraggingLayout = false;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->HandlePenUp(lastTouchPoint, focusManager);
    }
}

SharedPtr<View> DisplaySettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    int idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        if (currentFocus.GetPointer() == layoutOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                _isManualScroll = false;
                if (--idx < 0) idx = 0;
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                _isManualScroll = false;
                if (++idx >= (int)_layoutOptions.size()) idx = _layoutOptions.size() - 1;
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Down)
            {
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
                if (--idx < 0)
                    idx += _sortOptions.size();
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_sortOptions.size())
                    idx = 0;
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                _isManualScroll = false;
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

u32 DisplaySettingsBottomSheetView::LoadIcon(IVramManager& vramManager,
    const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}
