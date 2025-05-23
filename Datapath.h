#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    Datapath.h

Abstract:

    Заголовочный файл для модуля путей данных.
    Содержит объявления функций и структур, используемых в Datapath.c

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Максимальный размер пакета
#define I219V_MAX_PACKET_SIZE 16384

// Объявление функций для работы с путями данных
NTSTATUS I219vInitializeRxRing(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vInitializeTxRing(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vCleanupRings(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vInitializeDma(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vInitializeDatapath(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vCleanupDatapath(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
