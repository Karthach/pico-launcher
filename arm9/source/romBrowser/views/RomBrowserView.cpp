#include "common.h"
#include "IconGridItemView.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "RomBrowserView.h"

RomBrowserView::RomBrowserView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode& displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    VBlankTextureLoader* vblankTextureLoader)
    : _viewModel(std::move(viewModel)), _isVertical(displayMode.IsVertical())
{
    _fileGridView = displayMode.CreateRecyclerView(romBrowserViewFactory);
    AddChildTail(_fileGridView.GetPointer());
    _fileRecyclerAdapter = displayMode.CreateRecyclerAdapter(
        _viewModel.GetPointer(), themeFileIconFactory, romBrowserViewFactory, vblankTextureLoader);
}

void RomBrowserView::InitVram(const VramContext& vramContext)
{
    _fileRecyclerAdapter->InitVram(vramContext); // first initialize the shared vram for the items
    _fileGridView->SetAdapter(_fileRecyclerAdapter, _viewModel->GetSelectedItem()); // set the adapter of the recycler
    _fileGridView->InitVram(vramContext); // init the vram for the recycler and its items
}

void RomBrowserView::Update()
{
    _fileRecyclerAdapter->SetIconFrameCounter(_viewModel->GetIconFrameCounter());
    _fileGridView->Update();
    _viewModel->SetSelectedItem(_fileGridView->GetSelectedItem());
}

SharedPtr<View> RomBrowserView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == GetParent())
    {
        if (_isVertical)
        {
            if (direction == FocusMoveDirection::Right)
            {
                return _fileGridView->MoveFocus(currentFocus, direction, this);
            }
        }
        else
        {
            if (direction == FocusMoveDirection::Down)
            {
                return _fileGridView->MoveFocus(currentFocus, direction, this);
            }
        }
        return nullptr;
    }
    else if (source == _fileGridView.GetPointer())
    {
        return View::MoveFocus(currentFocus, direction, source);
    }
    return nullptr;
}

void RomBrowserView::JumpToLetter(char16_t letter, FocusManager& focusManager)
{
    if (!_fileRecyclerAdapter) return;
    
    // Prioritize new tasks by canceling old ones
    _viewModel->GetIoTaskQueue()->CancelAllTasks();

    u32 count = _viewModel->GetFileInfoManager().GetItemCount();
    int targetIdx = -1;
    for (u32 i = 0; i < count; i++)
    {
        const auto& item = _viewModel->GetFileInfoManager().GetItem(i);
        char16_t firstChar = 0;

        // Try title first if it exists
        auto internalInfo = _viewModel->GetFileInfoManager().GetInternalFileInfo(i);
        if (internalInfo)
        {
            const char16_t* title = internalInfo->GetGameTitle();
            if (title && title[0]) firstChar = title[0];
        }

        // Fallback to filename
        if (firstChar == 0)
        {
            firstChar = (char16_t)item.GetFileName()[0];
        }

        // Convert to uppercase for comparison if it's a latin letter
        if (firstChar >= 'a' && firstChar <= 'z') firstChar -= ('a' - 'A');
        
        if (firstChar >= letter)
        {
            targetIdx = i;
            break;
        }
    }

    if (targetIdx != -1)
    {
        _fileGridView->SetSelectedItem(targetIdx);
        _viewModel->SetSelectedItem(targetIdx);
        _fileGridView->Focus(focusManager);
    }
}
