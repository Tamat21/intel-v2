/*++

Copyright (c) 2025 Manus AI

Module Name:

    NetAdapterConfig.c

Abstract:

    This file previously contained functions for configuring and creating the NetAdapter.
    This logic has been consolidated into Driver.c (for NetAdapter_Init allocation and NetAdapterCreate)
    and Adapter.c (for the implementations of NetAdapter event callbacks like EvtAdapterSetCapabilities,
    EvtAdapterStart, EvtAdapterStop, EvtAdapterPause, EvtAdapterRestart).

    This file is now largely empty or could be removed if no other utility functions
    specific to NetAdapter configuration reside here.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>
#include "Driver.h"
#include "Device.h"
#include "Adapter.h"
#include "Queue.h"
#include "i219v_hw.h"
#include "Datapath.h"
#include "DeviceContext.h"
#include "Trace.h"

// All substantive functions previously in this file (I219vCreateNetAdapter,
// I219vEvtAdapterSetCapabilities, I219vEvtAdapterStart, I219vEvtAdapterStop)
// have been moved or their logic integrated into Adapter.c and Driver.c
// to consolidate the NetAdapter initialization path.

// If I219vInitializeInterrupt was only used by the old I219vEvtAdapterStart in this file,
// and is not called from the new consolidated I219vEvtAdapterStart (now in Adapter.c),
// it might also be orphaned. For now, its definition is assumed to be elsewhere if still needed
// (e.g. in i219v_hw.c or similar, and called during device D0Entry or PrepareHardware).
// The call to I219vInitializeInterrupt was commented out in the consolidated I219vEvtAdapterStart.
// If it's truly unused, it should be removed from its definition file.
// If it IS used by the new Start, its prototype needs to be available in Adapter.c.
// For this refactoring, we assume I219vInitializeInterrupt is handled elsewhere (e.g. D0Entry).

// The functions I219vEnableDevice, I219vDisableDevice, I219vPauseDevice, I219vRestartDevice
// were called from the EvtAdapterXXX handlers that were in this file.
// These are HW manipulation functions, likely implemented in i219v_hw.c and declared in Adapter.h or i219v_hw.h.
// The consolidated EvtAdapterXXX handlers in Adapter.c will continue to call them as appropriate.
