#include "common.h"
#include "App.h"
#include "services/process/ProcessManager.h"
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"

void PicoLoaderProcess::Run()
{
    if (!pload_start())
    {
        gProcessManager.Goto<App>();
    }
}
