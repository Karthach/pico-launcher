#include "common.h"
#include "RomBrowserHorizontalIconGridDisplayMode.h"
#include "RomBrowserVerticalIconGridDisplayMode.h"
#include "RomBrowserBannerListDisplayMode.h"
#include "RomBrowserHorizontalCoverFlowDisplayMode.h"
#include "RomBrowserInvertedCoverFlowDisplayMode.h"
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
        {
            return &RomBrowserBannerListDisplayMode::sInstance;
        }
        case RomBrowserLayout::CoverFlow:
        {
            return &RomBrowserHorizontalCoverFlowDisplayMode::sInstance;
        }
        case RomBrowserLayout::InvertedCoverFlow:
        {
            return &RomBrowserInvertedCoverFlowDisplayMode::sInstance;
        }
        default:
        {
            return nullptr;
        }
    }
}