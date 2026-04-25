#include "common.h"
#include "../viewModels/RomBrowserAppBarViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "picturesIcon.h"
#include "heartIcon.h"
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
    _appBarView = displayMode.CreateAppBarView(romBrowserViewFactory, 1, 2);
    AddChildTail(_appBarView.GetPointer());

    _appBarView->SetButtonAction(APP_BAR_BUTTON_BACK, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->NavigateUp();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_THEME_SETTINGS, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowThemeSettings();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_DISPLAY_SETTINGS, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowDisplaySettings();
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

        u32 themeIconVramOffset = objVramManager->Alloc(picturesIconTilesLen);
        dma_ntrCopy32(3, picturesIconTiles, objVramManager->GetVramAddress(themeIconVramOffset), picturesIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_THEME_SETTINGS, themeIconVramOffset);

        u32 settingsIconVramOffset = objVramManager->Alloc(settingsIconTilesLen);
        dma_ntrCopy32(3, settingsIconTiles, objVramManager->GetVramAddress(settingsIconVramOffset), settingsIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_DISPLAY_SETTINGS, settingsIconVramOffset);
    }
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
