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
    , _searchText(Label2DView::CreateShared(224, 16, 28, fontRepository->GetFont(FontType::Medium10)))
{
    _romBrowserAppBarView->SetParent(this);
    _alphabetBar->SetParent(this);
    static const char* searchKeys[30] = {
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
        "A", "S", "D", "F", "G", "H", "J", "K", "L",
        "Z", "X", "C", "V", "B", "N", "M",
        "DEL", "SPACE", "CLEAR", "OK"
    };
    for (u32 i = 0; i < _searchKeys.size(); i++)
    {
        _searchKeys[i] = Label2DView::CreateShared(i >= 26 ? 50 : 20, 16, 8, fontRepository->GetFont(FontType::Medium10));
        _searchKeys[i]->SetText(searchKeys[i]);
    }
}

void RomBrowserBottomScreenView::InitVram(const VramContext& vramContext)
{
    _romBrowserAppBarView->InitVram(vramContext);
    _alphabetBar->InitVram(vramContext);
    _searchText->InitVram(vramContext);
    for (auto& key : _searchKeys) key->InitVram(vramContext);
}

void RomBrowserBottomScreenView::Update()
{
    _romBrowserAppBarView->Update();
    if (_viewModel->GetRomBrowserController()->ConsumeSearchRequest())
    {
        StringUtil::Copy(_pendingSearch, _viewModel->GetRomBrowserController()->GetSearchQuery(), sizeof(_pendingSearch));
        _searchCursor = 0;
        _searchActive = true;
    }
    if (_searchActive)
    {
        UpdateSearchKeyboard();
        return;
    }
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->Update();
        _alphabetBar->Update();
    }
}

void RomBrowserBottomScreenView::Draw(GraphicsContext& graphicsContext)
{
    _romBrowserAppBarView->Draw(graphicsContext);
    if (_searchActive)
    {
        graphicsContext.SetClipArea(Rectangle(0, 0, 256, 192));
        for (auto& key : _searchKeys) key->Draw(graphicsContext);
        _searchText->Draw(graphicsContext);
        graphicsContext.ResetClipArea();
        return;
    }
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->Draw(graphicsContext);
        _alphabetBar->Draw(graphicsContext);
    }
}

void RomBrowserBottomScreenView::VBlank()
{
    _romBrowserAppBarView->VBlank();
    if (_searchActive)
    {
        _searchText->VBlank();
        for (auto& key : _searchKeys) key->VBlank();
    }
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
    if (_searchActive)
    {
        if (inputProvider.Triggered(InputKey::B))
        {
            _searchActive = false;
            return true;
        }
        if (inputProvider.Triggered(InputKey::A))
        {
            HandleSearchKey(_searchCursor);
            return true;
        }
        if (inputProvider.Triggered(InputKey::X))
        {
            const size_t length = strlen(_pendingSearch);
            if (length) _pendingSearch[length - 1] = 0;
            UpdateSearchKeyboard();
            return true;
        }
        if (inputProvider.Triggered(InputKey::Start))
        {
            SubmitSearch();
            return true;
        }
        if (inputProvider.Triggered(InputKey::DpadLeft)) _searchCursor = (_searchCursor + 29) % 30;
        if (inputProvider.Triggered(InputKey::DpadRight)) _searchCursor = (_searchCursor + 1) % 30;
        if (inputProvider.Triggered(InputKey::DpadUp)) _searchCursor = (_searchCursor + 20) % 30;
        if (inputProvider.Triggered(InputKey::DpadDown)) _searchCursor = (_searchCursor + 10) % 30;
        UpdateSearchKeyboard();
        return true;
    }
    if (inputProvider.Triggered(InputKey::B))
    {
        if (_viewModel->GetRomBrowserController()->IsFavoritesView())
            _viewModel->GetRomBrowserController()->ToggleFavoritesView();
        else
            _viewModel->NavigateUp();
        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}

void RomBrowserBottomScreenView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (_searchActive)
    {
        _searchPenDown = touchPoint;
        return;
    }
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
    if (_searchActive) return;
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
    if (_searchActive)
    {
        if (_searchPenDown.DistanceSquaredTo(lastTouchPoint) <= 64)
        {
            for (int i = 0; i < (int)_searchKeys.size(); i++)
            {
                if (_searchKeys[i]->GetBounds().Contains(lastTouchPoint))
                {
                    HandleSearchKey(i);
                    break;
                }
            }
        }
        return;
    }
    _romBrowserAppBarView->HandlePenUp(lastTouchPoint, focusManager);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
    {
        _romBrowserView->HandlePenUp(lastTouchPoint, focusManager);
    }
}

void RomBrowserBottomScreenView::UpdateSearchKeyboard()
{
    static const int rowX[] = { 18, 29, 51 };
    static const int rowY[] = { 58, 86, 114 };
    for (int i = 0; i < 26; i++)
    {
        const int row = i < 10 ? 0 : i < 19 ? 1 : 2;
        const int col = row == 0 ? i : row == 1 ? i - 10 : i - 19;
        _searchKeys[i]->SetPosition(rowX[row] + col * 22, rowY[row]);
    }
    for (int i = 26; i < 30; i++)
        _searchKeys[i]->SetPosition(8 + (i - 26) * 61, 148);

    const bool spanish = !strcasecmp(_viewModel->GetRomBrowserController()->GetLanguage(), "spanish");
    static const char* controlsEn[] = { "DEL", "SPACE", "CLEAR", "OK" };
    static const char* controlsEs[] = { "BORRAR", "ESPACIO", "LIMPIAR", "OK" };
    for (int i = 26; i < 30; i++)
        _searchKeys[i]->SetText((spanish ? controlsEs : controlsEn)[i - 26]);
    _searchText->SetText(_pendingSearch[0] ? _pendingSearch : (spanish ? "Buscar..." : "Search..."));
    _searchText->SetPosition(16, 24);
    _searchText->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _searchText->SetForegroundColor(_materialColorScheme->primary);
    for (int i = 0; i < (int)_searchKeys.size(); i++)
    {
        _searchKeys[i]->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _searchKeys[i]->SetForegroundColor(i == _searchCursor ? _materialColorScheme->primary : _materialColorScheme->onSurfaceVariant);
        _searchKeys[i]->Update();
    }
    _searchText->Update();
}

void RomBrowserBottomScreenView::HandleSearchKey(int index)
{
    const size_t length = strlen(_pendingSearch);
    if (index < 26)
    {
        if (length < sizeof(_pendingSearch) - 1)
        {
            _pendingSearch[length] = "QWERTYUIOPASDFGHJKLZXCVBNM"[index];
            _pendingSearch[length + 1] = 0;
        }
    }
    else if (index == 26)
    {
        if (length) _pendingSearch[length - 1] = 0;
    }
    else if (index == 27)
    {
        if (length < sizeof(_pendingSearch) - 1)
        {
            _pendingSearch[length] = ' ';
            _pendingSearch[length + 1] = 0;
        }
    }
    else if (index == 28)
        _pendingSearch[0] = 0;
    else if (index == 29)
    {
        SubmitSearch();
        return;
    }
    UpdateSearchKeyboard();
}

void RomBrowserBottomScreenView::SubmitSearch()
{
    _viewModel->GetRomBrowserController()->SetSearchQuery(_pendingSearch);
    _searchActive = false;
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
