#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    Device.h

Abstract:

    Заголовочный файл для модуля устройства.
    Содержит объявления функций и структур, используемых в Device.c

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Объявление функций для работы с устройством
EVT_WDF_DEVICE_PREPARE_HARDWARE I219vEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE I219vEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY I219vEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT I219vEvtDeviceD0Exit;

// Объявление функций для работы с адаптером
EVT_NET_ADAPTER_CREATE_TXQUEUE I219vEvtAdapterCreateTxQueue;
EVT_NET_ADAPTER_CREATE_RXQUEUE I219vEvtAdapterCreateRxQueue;
EVT_NET_ADAPTER_SET_CAPABILITIES I219vEvtAdapterSetCapabilities;
EVT_NET_ADAPTER_START I219vEvtAdapterStart;
EVT_NET_ADAPTER_STOP I219vEvtAdapterStop;
EVT_NET_ADAPTER_PAUSE I219vEvtAdapterPause;
EVT_NET_ADAPTER_RESTART I219vEvtAdapterRestart;

// Объявление вспомогательных функций
NTSTATUS I219vReadMacAddress(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vInitializeHardware(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vShutdownHardware(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
