/*++

Copyright (c) 2025 Manus AI

Module Name:

    Trace.h

Abstract:

    Заголовочный файл для трассировки и отладки драйвера.
    Содержит определения для WPP трассировки.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

// Определение GUID для WPP трассировки
// {D58C126F-B309-4C11-8497-6DF897AEEA3D}
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        I219vTraceGuid, (D58C126F,B309,4C11,8497,6DF897AEEA3D), \
        WPP_DEFINE_BIT(TRACE_DRIVER)     /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(TRACE_DEVICE)     /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(TRACE_ADAPTER)    /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(TRACE_QUEUE)      /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(TRACE_HARDWARE)   /* bit  4 = 0x00000010 */ \
        )

// Определение макросов для трассировки
#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
    WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

// Определение формата сообщений трассировки
#define WPP_FLAGS_LEVEL_STATUS_LOGGER(flags, lvl, status) \
    WPP_LEVEL_LOGGER(flags)

#define WPP_FLAGS_LEVEL_STATUS_ENABLED(flags, lvl, status) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

// Включение файлов трассировки
#include <wdf.h>
#include <WppRecorder.h>

// Определение макроса для трассировки
#define TraceEvents(level, flags, ...) \
    WPP_RECORDER_SF_L(flags, level, __VA_ARGS__)
