#include "common.h"
#include <string.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include "settings/SettingsController.h"
#include "services/localization/ILocalizationService.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"
#include "ThemeListTopView.h"

ThemeListTopView::ThemeListTopView(SharedPtr<ThemeListViewModel> viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, ILocalizationService& localizationService)
    : _viewModel(viewModel)
    , _localizationService(localizationService)
    , _noPreviewLabel(Label2DView::CreateShared(224, 16, 64, fontRepository->GetFont(FontType::Medium11)))
{
    _noPreviewLabel->SetHorizontalAlignment(Alignment::Center);
    _noPreviewLabel->SetPosition(16, 96 - 8);
    _noPreviewLabel->SetBackgroundColor(materialColorScheme->inverseOnSurface);
    _noPreviewLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(_noPreviewLabel.GetPointer());
}

void ThemeListTopView::VBlank()
{
    ViewContainer::VBlank();
    REG_DISPCNT_SUB = (REG_DISPCNT_SUB & ~0xF) | 5 | (4 << 8);
    REG_BG2CNT_SUB = 0x4084;
    REG_BG2HOFS_SUB = 0;
    REG_BG2VOFS_SUB = 0;
    gfx_setSubBg2Affine(256, 0, 0, 256, 0, 0);

    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem)
    {
        _lastSelectedItem = selectedItem;
        _selectionResolved = false;
    }
    if (_selectionResolved)
        return;

    if (selectedItem < 0)
    {
        memset((void*)GFX_BG_SUB, 0, 256 * 192 * 2);
        auto& themeInfoManager = _viewModel->GetSettingsController()->GetThemeInfoManager();
        _noPreviewLabel->SetText(themeInfoManager.GetItemCount() == 0
            ? _localizationService.GetString("theme_list_empty")
            : _localizationService.GetString("theme_preview_select"));
        _selectionResolved = true;
        return;
    }

    auto extraThemeInfo = _viewModel->GetSettingsController()->GetThemeInfoManager().GetExtraThemeInfo(selectedItem);
    if (!extraThemeInfo)
    {
        memset((void*)GFX_BG_SUB, 0, 256 * 192 * 2);
        _noPreviewLabel->SetText(_localizationService.GetString("theme_preview_loading"));
        return;
    }

    if (extraThemeInfo->hasPreview)
    {
        dma_ntrCopy32(3, extraThemeInfo->previewImage, GFX_BG_SUB, 256 * 192 * 2);
        _noPreviewLabel->SetText(u"");
    }
    else
    {
        memset((void*)GFX_BG_SUB, 0, 256 * 192 * 2);
        _noPreviewLabel->SetText(_localizationService.GetString("theme_preview_unavailable"));
    }
    _selectionResolved = true;
}
