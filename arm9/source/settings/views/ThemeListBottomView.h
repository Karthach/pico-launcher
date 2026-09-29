#pragma once
#include <memory>
#include "ThemeAdapter.h"
#include "gui/views/RecyclerView.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "settings/viewModels/ThemeListViewModel.h"
#include "settings/views/SettingsAppBarView.h"

class IFontRepository;
class ILocalizationService;
class ISettingsController;
class MaterialColorScheme;

class ThemeListBottomView : public ViewContainer
{
    SHARED_ONLY(ThemeListBottomView)

public:
    ThemeListBottomView(SharedPtr<ThemeListViewModel> viewModel, const MaterialColorScheme* materialColorScheme,
        const IRomBrowserViewFactory* romBrowserViewFactory, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader, const IFontRepository* fontRepository,
        ILocalizationService& localizationService);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 256, 192);
    }

    void Focus(FocusManager& focusManager)
    {
        if (_themeAdapter->GetItemCount() == 0)
            _appBarView->Focus(focusManager);
        else
            _recyclerView->Focus(focusManager);
    }

    SharedPtr<SettingsAppBarView> _appBarView;
private:
    SharedPtr<RecyclerView> _recyclerView;
    SharedPtr<ThemeAdapter> _themeAdapter;
    SharedPtr<ThemeListViewModel> _viewModel;
    SharedPtr<Label2DView> _emptyStateLabel;
};
