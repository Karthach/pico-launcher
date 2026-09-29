#include "common.h"
#include <nds/arm9/video.h>
#include <nds/arm9/cache.h>
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include "core/Environment.h"
#include <string.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/dma/dmaTwl.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include "ipcChannels.h"
#include "fat/File.h"
#include "core/StringUtil.h"
#include "picoLoaderBootstrap.h"

#define PICO_LOADER_9_PATH    "/_pico/picoLoader9.bin"
#define PICO_LOADER_7_PATH    "/_pico/picoLoader7.bin"
#define PICO_LOADER_9_MAX_SIZE 0x40000
#define PICO_LOADER_7_MAX_SIZE 0x20000

static bool readLoaderFile(const char* path, u32 minimumSize, u32 maximumSize,
    std::unique_ptr<u8[]>& buffer, u32& fileSize)
{
    File file;
    if (file.Open(path, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return false;

    fileSize = file.GetSize();
    if (fileSize < minimumSize || fileSize > maximumSize)
        return false;

    u32 transferSize = (fileSize + 1) & ~1u;
    buffer.reset(new(cache_align) u8[transferSize]);
    memset(buffer.get(), 0, transferSize);
    return file.ReadExact(buffer.get(), fileSize);
}

typedef void (*pico_loader_9_func_t)(void);

static pload_params_t sLoadParams;
static char sLauncherPath[256] alignas(32);
static PicoLoaderBootDrive sBootDrive;
static const pload_cheats_t* sCheatData = nullptr;

pload_params_t* pload_getLoadParams()
{
    return &sLoadParams;
}

void pload_setBootDrive(PicoLoaderBootDrive bootDrive)
{
    sBootDrive = bootDrive;
}

void pload_setLauncherPath(const char* launcherPath)
{
    StringUtil::Copy(sLauncherPath, launcherPath, sizeof(sLauncherPath));
}

void pload_setCheatData(const pload_cheats_t* cheatData)
{
    sCheatData = cheatData;
}

bool pload_start()
{
    std::unique_ptr<u8[]> picoLoader9;
    std::unique_ptr<u8[]> picoLoader7;
    u32 picoLoader9Size = 0;
    u32 picoLoader7Size = 0;
    constexpr u32 picoLoader7RequiredSize = offsetof(pload_header7_t, loadParams) + sizeof(pload_params_t);

    if (!readLoaderFile(PICO_LOADER_9_PATH, 4, PICO_LOADER_9_MAX_SIZE, picoLoader9, picoLoader9Size) ||
        !readLoaderFile(PICO_LOADER_7_PATH, picoLoader7RequiredSize, PICO_LOADER_7_MAX_SIZE, picoLoader7, picoLoader7Size))
    {
        LOG_ERROR("Pico Loader files are missing, truncated, or too large.\n");
        return false;
    }

    auto loaderHeader = reinterpret_cast<const pload_header7_t*>(picoLoader7.get());
    uintptr_t arm7EntryPoint = (uintptr_t)loaderHeader->entryPoint;
    if (loaderHeader->apiVersion == 0 || loaderHeader->apiVersion > PICO_LOADER_API_VERSION ||
        // The file is copied through ARM9's LCDC window at 0x06840000, but
        // ARM7 executes VRAM C through its own 0x06000000 mapping.
        arm7EntryPoint < 0x06000000 || arm7EntryPoint >= 0x06000000 + picoLoader7Size)
    {
        LOG_ERROR("Pico Loader ARM7 header is invalid or unsupported.\n");
        return false;
    }

    u32 requiredSize = picoLoader7RequiredSize;
    if (loaderHeader->apiVersion >= 3)
        requiredSize = offsetof(pload_header7_t, v3) + sizeof(pload_header7_v3_t);
    else if (loaderHeader->apiVersion >= 2)
        requiredSize = offsetof(pload_header7_t, v2) + sizeof(pload_header7_v2_t);
    if (picoLoader7Size < requiredSize)
    {
        LOG_ERROR("Pico Loader ARM7 file is truncated for its API version.\n");
        return false;
    }

    REG_MASTER_BRIGHT = 0x401F;
    REG_MASTER_BRIGHT_SUB = 0x401F;
    REG_DISPCNT = 0;
    REG_DISPCNT_SUB = 0;

    mem_setVramAMapping(MEM_VRAM_AB_LCDC);
    mem_setVramCMapping(MEM_VRAM_C_LCDC);
    mem_setVramDMapping(MEM_VRAM_D_LCDC);

    DC_FlushRange(picoLoader9.get(), picoLoader9Size);
    dma_ntrCopy16(3, picoLoader9.get(), (void*)0x06800000, (picoLoader9Size + 1) & ~1);
    DC_FlushRange(picoLoader7.get(), picoLoader7Size);
    dma_ntrCopy16(3, picoLoader7.get(), (void*)0x06840000, (picoLoader7Size + 1) & ~1);

    rtos_disableIrqs();
    REG_IME = 0;

    dma_ntrStopSafe(0);
    dma_ntrStopSafe(1);
    dma_ntrStopSafe(2);
    dma_ntrStopSafe(3);

    if (Environment::IsDsiMode())
    {
        REG_NDMA0CNT = 0;
        REG_NDMA1CNT = 0;
        REG_NDMA2CNT = 0;
        REG_NDMA3CNT = 0;
    }

    DC_FlushAll();
    DC_InvalidateAll();
    IC_InvalidateAll();

    auto header = (pload_header7_t*)0x06840000;
    header->bootDrive = sBootDrive;
    dma_ntrCopy16(3, &sLoadParams, &header->loadParams, sizeof(pload_params_t));
    if (header->apiVersion >= 2)
    {
        dma_ntrCopy16(3, &sLauncherPath, &header->v2.launcherPath, sizeof(header->v2.launcherPath));
    }
    if (header->apiVersion >= 3)
    {
        header->v3.cheats = sCheatData;
    }
    mem_setVramCMapping(MEM_VRAM_C_ARM7_00000);
    mem_setVramDMapping(MEM_VRAM_D_ARM7_20000);
    ipc_sendFifoMessage(IPC_CHANNEL_LOADER, 1);
    ((pico_loader_9_func_t)0x06800000)();
    LOG_ERROR("Pico Loader returned unexpectedly.\n");
    rtos_enableIrqs();
    REG_IME = 1;
    return false;
}
