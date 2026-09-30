#include "common.h"
#include "../viewModels/RomBrowserAppBarViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "heartIcon.h"
#include "searchIcon.h"
#include "starIcon.h"
#include "recentIcon.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "coverflowIcon.h"
#include "listIcon.h"
#include "moviesIcon.h"
#include "gui/IVramManager.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "RomBrowserAppBarView.h"

RomBrowserAppBarView::RomBrowserAppBarView(
    RomBrowserAppBarViewModel* viewModel, const RomBrowserDisplayMode& displayMode,
    const IRomBrowserViewFactory* romBrowserViewFactory)
    : _viewModel(viewModel)
{
    _appBarView = displayMode.CreateAppBarView(romBrowserViewFactory, 1, 3);
    AddChildTail(_appBarView.GetPointer());

    _appBarView->SetButtonAction(APP_BAR_BUTTON_BACK, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->NavigateUp();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_DISPLAY_SETTINGS, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowDisplaySettings();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_SEARCH, [] (IconButtonView*, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowSearch();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_FAVORITES, [] (IconButtonView*, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ToggleFavorites();
    }, _viewModel);
}

void RomBrowserAppBarView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        u32 backIconVramOffset = objVramManager->Alloc(backIconTilesLen);
        dma_ntrCopy32(3, backIconTiles, objVramManager->GetVramAddress(backIconVramOffset), backIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_BACK, backIconVramOffset);

        u32 settingsIconVramOffset = objVramManager->Alloc(settingsIconTilesLen);
        dma_ntrCopy32(3, settingsIconTiles, objVramManager->GetVramAddress(settingsIconVramOffset), settingsIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_DISPLAY_SETTINGS, settingsIconVramOffset);

        u32 searchIconVramOffset = objVramManager->Alloc(searchIconTilesLen);
        dma_ntrCopy32(3, searchIconTiles, objVramManager->GetVramAddress(searchIconVramOffset), searchIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_SEARCH, searchIconVramOffset);

        u32 starIconVramOffset = objVramManager->Alloc(starIconTilesLen);
        dma_ntrCopy32(3, starIconTiles, objVramManager->GetVramAddress(starIconVramOffset), starIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_FAVORITES, starIconVramOffset);
    }
}

void RomBrowserAppBarView::Update()
{
    _appBarView->SetButtonState(APP_BAR_BUTTON_FAVORITES,
        _viewModel->IsFavoritesView() ? IconButtonView::State::ToggleSelected : IconButtonView::State::NoToggle);
    ViewContainer::Update();
}

SharedPtr<View> RomBrowserAppBarView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == _appBarView.GetPointer())
    {
        return View::MoveFocus(currentFocus, direction, source);
    }
    else if (source == GetParent())
    {
        return _appBarView->MoveFocus(currentFocus, direction, this);
    }
    return nullptr;
}
