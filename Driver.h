#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    Driver.h

Abstract:

    Заголовочный файл для основного модуля драйвера.
    Содержит объявления функций и структур, используемых в Driver.c

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Объявление функций
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD I219vEvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD I219vEvtDeviceAdd;

// Определение уровней трассировки
typedef enum _TRACE_LEVEL {
    TRACE_LEVEL_NONE        = 0,
    TRACE_LEVEL_CRITICAL    = 1,
    TRACE_LEVEL_ERROR       = 2,
    TRACE_LEVEL_WARNING     = 3,
    TRACE_LEVEL_INFORMATION = 4,
    TRACE_LEVEL_VERBOSE     = 5,
} TRACE_LEVEL;

// Определение компонентов для трассировки
#define TRACE_DRIVER        0x00000001
#define TRACE_DEVICE        0x00000002
#define TRACE_ADAPTER       0x00000004
#define TRACE_QUEUE         0x00000008
#define TRACE_HARDWARE      0x00000010

// Глобальные переменные
extern DRIVER_OBJECT* g_DriverObject;
