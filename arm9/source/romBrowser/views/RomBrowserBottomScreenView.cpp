#include "common.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../views/IconGridItemView.h"
#include "gui/GraphicsContext.h"
#include "gui/views/Label2DView.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "heartIcon.h"
#include "recentIcon.h"
#include "listIcon.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "RomBrowserBottomScreenView.h"

RomBrowserBottomScreenView::RomBrowserBottomScreenView(
    RomBrowserBottomScreenViewModel* viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const IFontRepository* fontRepository,
    const MaterialColorScheme* materialColorScheme,
    VBlankTextureLoader* vblankTextureLoader)
    : _viewModel(viewModel)
    , _romBrowserViewFactory(romBrowserViewFactory)
    , _romBrowserDisplayMode(displayMode)
    , _themeFileIconFactory(themeFileIconFactory)
    , _fontRepository(fontRepository)
    , _materialColorScheme(materialColorScheme)
    , _romBrowserAppBarView(RomBrowserAppBarView::CreateShared(_viewModel->GetRomBrowserAppBarViewModel(),
        *displayMode, romBrowserViewFactory))
    , _alphabetBar(SharedPtr<AlphabetBar>::MakeShared(this, fontRepository, materialColorScheme))
    , _vblankTextureLoader(vblankTextureLoader)
{
    _romBrowserAppBarView->SetParent(this);
    _alphabetBar->SetParent(this);
}

void RomBrowserBottomScreenView::InitVram(const VramContext& vramContext)
{
    _romBrowserAppBarView->InitVram(vramContext);
    _alphabetBar->InitVram(vramContext);
}

void RomBrowserBottomScreenView::Update()
{
    _romBrowserAppBarView->Update();
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->Update();
        _alphabetBar->Update();
    }
}

void RomBrowserBottomScreenView::Draw(GraphicsContext& graphicsContext)
{
    _romBrowserAppBarView->Draw(graphicsContext);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->Draw(graphicsContext);
        _alphabetBar->Draw(graphicsContext);
    }
}

void RomBrowserBottomScreenView::VBlank()
{
    _romBrowserAppBarView->VBlank();
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->VBlank();
        _alphabetBar->VBlank();
    }
}

SharedPtr<View> RomBrowserBottomScreenView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == _romBrowserAppBarView.GetPointer())
    {
        if (_romBrowserDisplayMode->IsVertical())
        {
            if (direction == FocusMoveDirection::Right)
            {
                return _romBrowserView->MoveFocus(currentFocus, direction, this);
            }
        }
        else
        {
            if (direction == FocusMoveDirection::Down)
            {
                return _romBrowserView->MoveFocus(currentFocus, direction, this);
            }
        }
        return nullptr;
    }
    else if (source == _romBrowserView.GetPointer())
    {
        if (_romBrowserDisplayMode->IsVertical())
        {
            if (direction == FocusMoveDirection::Left)
            {
                return _romBrowserAppBarView->MoveFocus(currentFocus, direction, this);
            }
        }
        else
        {
            if (direction == FocusMoveDirection::Up)
            {
                return _romBrowserAppBarView->MoveFocus(currentFocus, direction, this);
            }
        }
        return nullptr;
    }
    return nullptr;
}

bool RomBrowserBottomScreenView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->NavigateUp();
        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}

void RomBrowserBottomScreenView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _romBrowserAppBarView->HandlePenDown(touchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        if (_alphabetBar->GetBounds().Contains(touchPoint))
        {
            _alphabetBar->HandlePenDown(touchPoint, focusManager);
        }
        else
        {
            _romBrowserView->HandlePenDown(touchPoint, focusManager);
        }
    }
}

void RomBrowserBottomScreenView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _romBrowserAppBarView->HandlePenMove(touchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        if (_alphabetBar->GetBounds().Contains(touchPoint))
        {
            _alphabetBar->HandlePenMove(touchPoint, focusManager);
        }
        else
        {
            _romBrowserView->HandlePenMove(touchPoint, focusManager);
        }
    }
}

void RomBrowserBottomScreenView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _romBrowserAppBarView->HandlePenUp(lastTouchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->HandlePenUp(lastTouchPoint, focusManager);
    }
}

void RomBrowserBottomScreenView::RomBrowserViewModelInvalidated(const VramContext& vramContext)
{
    if (_viewModel->GetRomBrowserViewModel().IsValid())
    {
        _romBrowserView = RomBrowserView::CreateShared(
            _viewModel->GetRomBrowserViewModel(), *_romBrowserDisplayMode,
            _themeFileIconFactory, _romBrowserViewFactory, _vblankTextureLoader);
        _romBrowserView->SetParent(this);
        _romBrowserView->InitVram(vramContext);
    }
    else
    {
        _romBrowserView.Reset();
    }
}

RomBrowserBottomScreenView::AlphabetBar::AlphabetBar(RomBrowserBottomScreenView* parent, const IFontRepository* fontRepository, const MaterialColorScheme* materialColorScheme)
    : _parent(parent), _materialColorScheme(materialColorScheme)
{
    const nft2_header_t* font = fontRepository->GetFont(FontType::Medium7_5);
    for (u32 i = 0; i < 26; i++)
    {
        _letterLabels[i] = Label2DView::CreateShared(12, 10, 2, font);
        char16_t letter[] = { (char16_t)(u'A' + i), 0 };
        _letterLabels[i]->SetText(letter);
        _letterLabels[i]->SetHorizontalAlignment(Alignment::Center);
        AddChildTail(_letterLabels[i].GetPointer());
    }
}

void RomBrowserBottomScreenView::AlphabetBar::InitVram(const VramContext& vramContext)
{
    for (auto& label : _letterLabels)
    {
        label->InitVram(vramContext);
    }
}

void RomBrowserBottomScreenView::AlphabetBar::Update()
{
    Rectangle bounds = GetBounds();
    // Always horizontal now
    int step = bounds.GetWidth() / 26;
    
    for (u32 i = 0; i < 26; i++)
    {
        _letterLabels[i]->SetPosition(bounds.GetX() + (i * step), bounds.GetY() + 1);
        _letterLabels[i]->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _letterLabels[i]->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _letterLabels[i]->Update();
    }
}

void RomBrowserBottomScreenView::AlphabetBar::Draw(GraphicsContext& graphicsContext)
{
    // Draw letters.
    for (auto& label : _letterLabels)
    {
        label->Draw(graphicsContext);
    }
}

void RomBrowserBottomScreenView::AlphabetBar::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    JumpToPoint(touchPoint, focusManager);
}

void RomBrowserBottomScreenView::AlphabetBar::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    JumpToPoint(touchPoint, focusManager);
}

void RomBrowserBottomScreenView::AlphabetBar::JumpToPoint(const Point& touchPoint, FocusManager& focusManager)
{
    Rectangle bounds = GetBounds();
    if (!bounds.Contains(touchPoint)) return;

    // Always horizontal mapping
    int letterIndex = ((touchPoint.x - bounds.GetX()) * 26) / bounds.GetWidth();

    if (letterIndex < 0) letterIndex = 0;
    if (letterIndex > 25) letterIndex = 25;

    char16_t letter = u'A' + letterIndex;
    _parent->_romBrowserView->JumpToLetter(letter, focusManager);
}

Rectangle RomBrowserBottomScreenView::AlphabetBar::GetBounds() const
{
    // Always horizontal bar at the bottom
    return Rectangle(40, 180, 210, 12);
}
