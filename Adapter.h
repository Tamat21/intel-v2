#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    Adapter.h

Abstract:

    Заголовочный файл для модуля сетевого адаптера.
    Содержит объявления функций и структур, используемых в Adapter.c

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Константы для адаптера
#define I219V_MAX_LINK_SPEED NDIS_LINK_SPEED_1000MBPS
#define I219V_MIN_LINK_SPEED NDIS_LINK_SPEED_10MBPS

// Объявление функций для управления устройством
VOID I219vEnableDevice(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vDisableDevice(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vPauseDevice(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vRestartDevice(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
