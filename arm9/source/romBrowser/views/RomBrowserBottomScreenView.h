#pragma once
#include "AppBarView.h"
#include "gui/views/View.h"
#include "gui/views/RecyclerView.h"
#include "../FileRecyclerAdapter.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "RomBrowserView.h"
#include "RomBrowserAppBarView.h"
#include "../viewModels/RomBrowserBottomScreenViewModel.h"

class IRomBrowserViewFactory;
class VBlankTextureLoader;
class IFontRepository;
class Label2DView;
class MaterialColorScheme;

class RomBrowserBottomScreenView : public View
{
    SHARED_ONLY(RomBrowserBottomScreenView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    void Focus(FocusManager& focusManager)
    {
        if (!_romBrowserView || !_romBrowserView->Focus(focusManager))
        {
            _romBrowserAppBarView->Focus(focusManager);
        }
    }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    void RomBrowserViewModelInvalidated(const VramContext& vramContext);

    bool IsAppBarFocused(const FocusManager& focusManager) const
    {
        return focusManager.IsFocusInside(_romBrowserAppBarView.GetPointer());
    }

    static SharedPtr<RomBrowserBottomScreenView> CreateShared(
        RomBrowserBottomScreenViewModel* viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        const IFontRepository* fontRepository,
        const MaterialColorScheme* materialColorScheme,
        VBlankTextureLoader* vblankTextureLoader)
    {
        return SharedPtr<RomBrowserBottomScreenView>::MakeShared(
            viewModel, displayMode, themeFileIconFactory, romBrowserViewFactory, fontRepository, materialColorScheme, vblankTextureLoader);
    }

private:
    class AlphabetBar : public ViewContainer
    {
    public:
        AlphabetBar(RomBrowserBottomScreenView* parent, const IFontRepository* fontRepository, const MaterialColorScheme* materialColorScheme);
        void InitVram(const VramContext& vramContext) override;
        void Update() override;
        void Draw(GraphicsContext& graphicsContext) override;
        void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
        void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
        Rectangle GetBounds() const override;

    private:
        RomBrowserBottomScreenView* _parent;
        const MaterialColorScheme* _materialColorScheme;
        std::array<SharedPtr<Label2DView>, 26> _letterLabels;
        void JumpToPoint(const Point& touchPoint, FocusManager& focusManager);
    };

    RomBrowserBottomScreenViewModel* _viewModel;
    const IRomBrowserViewFactory* _romBrowserViewFactory;

    const RomBrowserDisplayMode* _romBrowserDisplayMode;
    const IThemeFileIconFactory* _themeFileIconFactory;
    const IFontRepository* _fontRepository;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<RomBrowserAppBarView> _romBrowserAppBarView;
    SharedPtr<RomBrowserView> _romBrowserView;
    SharedPtr<AlphabetBar> _alphabetBar;
    VBlankTextureLoader* _vblankTextureLoader;

    RomBrowserBottomScreenView(
        RomBrowserBottomScreenViewModel* viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        const IFontRepository* fontRepository,
        const MaterialColorScheme* materialColorScheme,
        VBlankTextureLoader* vblankTextureLoader);
};
