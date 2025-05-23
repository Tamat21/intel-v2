#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_offload.h

Abstract:

    Заголовочный файл для модуля аппаратных оффлоадов Intel i219-v.
    Содержит объявления функций и структур для работы с аппаратными оффлоадами.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Структура статистики оффлоадов
typedef struct _I219V_OFFLOAD_STATISTICS {
    // Статистика оффлоада контрольной суммы
    UINT64 IpChecksumOffloadSuccesses;
    UINT64 IpChecksumOffloadFailures;
    UINT64 TcpChecksumOffloadSuccesses;
    UINT64 TcpChecksumOffloadFailures;
    UINT64 UdpChecksumOffloadSuccesses;
    UINT64 UdpChecksumOffloadFailures;
    
    // Статистика оффлоада сегментации TCP
    UINT64 TsoPackets;
    UINT64 TsoBytes;
    UINT64 TsoFailures;
    
    // Статистика оффлоада VLAN
    UINT64 VlanPackets;
    UINT64 VlanFailures;
} I219V_OFFLOAD_STATISTICS, *PI219V_OFFLOAD_STATISTICS;

// Объявление функций для работы с аппаратными оффлоадами
NTSTATUS I219vInitializeOffloads(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vConfigureTso(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vConfigureVlan(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vSetVlanFilter(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ UINT16 VlanId, _In_ BOOLEAN Enable);
NTSTATUS I219vSetChecksumOffload(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN EnableIpChecksum, _In_ BOOLEAN EnableTcpChecksum, _In_ BOOLEAN EnableUdpChecksum);
NTSTATUS I219vSetTsoOffload(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN EnableTso);
NTSTATUS I219vSetVlanOffload(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN EnableVlanOffload);
VOID I219vGetOffloadStatistics(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _Out_ PI219V_OFFLOAD_STATISTICS OffloadStatistics);
