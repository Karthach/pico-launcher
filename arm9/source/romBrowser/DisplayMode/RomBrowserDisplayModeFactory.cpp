#include "common.h"
#include "RomBrowserHorizontalIconGridDisplayMode.h"
#include "RomBrowserVerticalIconGridDisplayMode.h"
#include "RomBrowserBannerListDisplayMode.h"
#include "RomBrowserDisplayModeFactory.h"

const RomBrowserDisplayMode* RomBrowserDisplayModeFactory::GetRomBrowserDisplayMode(
    RomBrowserLayout romBrowserDisplayMode) const
{
    switch (romBrowserDisplayMode)
    {
        case RomBrowserLayout::HorizontalIconGrid:
        {
            return &RomBrowserHorizontalIconGridDisplayMode::sInstance;
        }
        case RomBrowserLayout::VerticalIconGrid:
        {
            return &RomBrowserVerticalIconGridDisplayMode::sInstance;
        }
        case RomBrowserLayout::BannerList:
        case RomBrowserLayout::FileList:
        {
            return &RomBrowserBannerListDisplayMode::sInstance;
        }
        case RomBrowserLayout::CoverFlow:
        case RomBrowserLayout::InvertedCoverFlow:
        {
            // Migrate legacy carousel settings to a supported layout.
            return &RomBrowserBannerListDisplayMode::sInstance;
        }
        default:
        {
            return nullptr;
        }
    }
}
