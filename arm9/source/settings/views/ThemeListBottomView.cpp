#include "common.h"
#include "gui/input/InputProvider.h"
#include "services/localization/ILocalizationService.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"
#include "ThemeListBottomView.h"

ThemeListBottomView::ThemeListBottomView(SharedPtr<ThemeListViewModel> viewModel, const MaterialColorScheme* materialColorScheme,
    const IRomBrowserViewFactory* romBrowserViewFactory, const IThemeFileIconFactory* themeFileIconFactory,
    VBlankTextureLoader* vblankTextureLoader, const IFontRepository* fontRepository,
    ILocalizationService& localizationService)
    : _appBarView(SettingsAppBarView::CreateShared(viewModel, romBrowserViewFactory))
    , _recyclerView(RecyclerView::CreateShared(42, 0, 256 - 42, 192, RecyclerView::Mode::VerticalList))
    , _viewModel(std::move(viewModel))
{
    AddChildTail(_appBarView.GetPointer());
    AddChildTail(_recyclerView.GetPointer());
    _recyclerView->SetPadding(0, 3);
    _recyclerView->SetItemSpacing(0, 3);
    _themeAdapter = SharedPtr<ThemeAdapter>::MakeShared(
        _viewModel->GetSettingsController(), romBrowserViewFactory, themeFileIconFactory, vblankTextureLoader);
    if (_themeAdapter->GetItemCount() == 0)
    {
        _emptyStateLabel = Label2DView::CreateShared(208, 32, 25, fontRepository->GetFont(FontType::Regular10));
        _emptyStateLabel->SetHorizontalAlignment(Alignment::Center);
        _emptyStateLabel->SetText(localizationService.GetString("theme_list_empty"));
        _emptyStateLabel->SetPosition(44, 80);
        _emptyStateLabel->SetBackgroundColor(materialColorScheme->inverseOnSurface);
        _emptyStateLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
        AddChildTail(_emptyStateLabel.GetPointer());
    }
}

void ThemeListBottomView::InitVram(const VramContext& vramContext)
{
    _appBarView->InitVram(vramContext);
    _themeAdapter->InitVram(vramContext); // first initialize the shared vram for the items
    _recyclerView->SetAdapter(_themeAdapter, _viewModel->GetSelectedItem()); // set the adapter of the recycler
    _recyclerView->InitVram(vramContext); // init the vram for the recycler and its items
    if (_emptyStateLabel)
        _emptyStateLabel->InitVram(vramContext);
}

void ThemeListBottomView::Update()
{
    ViewContainer::Update();
    _viewModel->SetSelectedItem(_recyclerView->GetSelectedItem());
    _appBarView->SetDeleteEnabled(_viewModel->CanDeleteSelected());
}

SharedPtr<View> ThemeListBottomView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == _appBarView.GetPointer())
    {
        if (direction == FocusMoveDirection::Right && _themeAdapter->GetItemCount() > 0)
        {
            return _recyclerView->MoveFocus(currentFocus, direction, this);
        }
        return nullptr;
    }
    else if (source == _recyclerView.GetPointer())
    {
        if (direction == FocusMoveDirection::Left)
        {
            return _appBarView->MoveFocus(currentFocus, direction, this);
        }
        return nullptr;
    }
    return nullptr;
}

bool ThemeListBottomView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->NavigateUp();
        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}
