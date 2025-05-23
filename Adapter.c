/*++

Copyright (c) 2025 Manus AI

Module Name:

    Adapter.c

Abstract:

    Реализация функций сетевого адаптера Intel i219-v с поддержкой игровых оптимизаций.
    Включает обработчики событий адаптера и настройку параметров.
    Расширено для поддержки игровых функций и оптимизаций Killer Performance.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>
#include "Driver.h"
#include "Device.h"
#include "Adapter.h"
#include "i219v_hw.h"
#include "i219v_hw_extended.h"
#include "i219v_gaming.h"
#include "DeviceContext.h"
#include "Trace.h"

// Обработчик установки возможностей канального уровня
VOID
I219vEvtAdapterSetLinkLayerCapabilities(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_LINK_LAYER_CAPABILITIES LinkLayerCapabilities
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting link layer capabilities");

    // Настройка возможностей канального уровня
    LinkLayerCapabilities->MaximumSendFrameSize = 1514;
    LinkLayerCapabilities->MaximumReceiveFrameSize = 1514;
    LinkLayerCapabilities->MtuSize = 1500;

    // Если включена поддержка Jumbo Frames
    if (deviceContext->MTU > 1500) {
        LinkLayerCapabilities->MaximumSendFrameSize = deviceContext->MTU + 14;
        LinkLayerCapabilities->MaximumReceiveFrameSize = deviceContext->MTU + 14;
        LinkLayerCapabilities->MtuSize = deviceContext->MTU;
    }

    // Настройка поддерживаемых фильтров пакетов
    LinkLayerCapabilities->SupportedPacketFilters =
        NetPacketFilterFlagDirected |
        NetPacketFilterFlagMulticast |
        NetPacketFilterFlagBroadcast |
        NetPacketFilterFlagPromiscuous |
        NetPacketFilterFlagAllMulticast;

    // Настройка максимального размера списка мультикаст-адресов
    LinkLayerCapabilities->MaximumMulticastListSize = 16;
}

// Обработчик установки адреса канального уровня
VOID
I219vEvtAdapterSetLinkLayerAddress(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_LINK_LAYER_ADDRESS LinkLayerAddress
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting link layer address");

    // Установка MAC-адреса
    LinkLayerAddress->Length = 6;
    RtlCopyMemory(LinkLayerAddress->Address, deviceContext->MacAddress, 6);
}

// Обработчик установки постоянного адреса канального уровня
VOID
I219vEvtAdapterSetPermanentLinkLayerAddress(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_LINK_LAYER_ADDRESS LinkLayerAddress
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting permanent link layer address");

    // Установка постоянного MAC-адреса
    LinkLayerAddress->Length = 6;
    RtlCopyMemory(LinkLayerAddress->Address, deviceContext->MacAddress, 6);
}

// Обработчик установки возможностей питания
VOID
I219vEvtAdapterSetPowerCapabilities(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_POWER_CAPABILITIES PowerCapabilities
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting power capabilities");

    // Настройка возможностей питания
    PowerCapabilities->SupportedWakePatterns =
        NetWakePatternFlagBitmapPattern |
        NetWakePatternFlagMagicPacket;

    // Если включено интеллектуальное управление энергопотреблением
    if (deviceContext->SmartPowerManagementEnabled) {
        PowerCapabilities->SupportedWakePatterns |= NetWakePatternFlagWakeOnMediaDisconnect;
        PowerCapabilities->SupportedProtocolOffloads |= NetProtocolOffloadFlagArpNs;
    }
}

// Обработчик установки возможностей DMA
VOID
I219vEvtAdapterSetDmaCapabilities(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_DMA_CAPABILITIES DmaCapabilities
    )
{
    UNREFERENCED_PARAMETER(Adapter);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting DMA capabilities");

    // Настройка возможностей DMA
    DmaCapabilities->MaximumPhysicalAddress.QuadPart = MAXULONG64;
    DmaCapabilities->PreferredNode = MM_ANY_NODE_OK;
}

// Обработчик установки возможностей приема
VOID
I219vEvtAdapterSetReceiveCapabilities(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_RECEIVE_CAPABILITIES ReceiveCapabilities
    )
{
    UNREFERENCED_PARAMETER(Adapter);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting receive capabilities");

    // Настройка возможностей приема
    ReceiveCapabilities->MaximumReceiveQueueCount = 1;
    ReceiveCapabilities->MaximumReceiveQueueGroupCount = 1;
}

// Обработчик установки возможностей оффлоадов
VOID
I219vEvtAdapterSetOffloadCapabilities(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_OFFLOAD_CAPABILITIES OffloadCapabilities
    )
{
    UNREFERENCED_PARAMETER(Adapter);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting offload capabilities");

    // Настройка возможностей оффлоадов
    OffloadCapabilities->SupportedChecksumOffloads =
        NetAdapterOffloadChecksumFlagIPv4Transmit |
        NetAdapterOffloadChecksumFlagTcpTransmit |
        NetAdapterOffloadChecksumFlagUdpTransmit |
        NetAdapterOffloadChecksumFlagIPv4Receive |
        NetAdapterOffloadChecksumFlagTcpReceive |
        NetAdapterOffloadChecksumFlagUdpReceive;

    OffloadCapabilities->SupportedLsoOffloads =
        NetAdapterOffloadLsoFlagIPv4 |
        NetAdapterOffloadLsoFlagIPv6;
}

// Обработчик установки текущего состояния соединения
VOID
I219vEvtAdapterSetCurrentLinkState(
    _In_ NETADAPTER Adapter,
    _Inout_ PNET_ADAPTER_LINK_STATE LinkState
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting current link state");

    // Настройка текущего состояния соединения
    if (deviceContext->LinkUp) {
        LinkState->MediaConnectState = MediaConnectStateConnected;
        
        // Установка скорости соединения
        switch (deviceContext->LinkSpeed) {
        case 1000:
            LinkState->MediaDuplexState = MediaDuplexStateFull;
            LinkState->XmitLinkSpeed = 1000000000;
            LinkState->RcvLinkSpeed = 1000000000;
            break;
        case 100:
            LinkState->MediaDuplexState = deviceContext->FullDuplex ? MediaDuplexStateFull : MediaDuplexStateHalf;
            LinkState->XmitLinkSpeed = 100000000;
            LinkState->RcvLinkSpeed = 100000000;
            break;
        case 10:
            LinkState->MediaDuplexState = deviceContext->FullDuplex ? MediaDuplexStateFull : MediaDuplexStateHalf;
            LinkState->XmitLinkSpeed = 10000000;
            LinkState->RcvLinkSpeed = 10000000;
            break;
        default:
            LinkState->MediaDuplexState = MediaDuplexStateFull;
            LinkState->XmitLinkSpeed = 1000000000;
            LinkState->RcvLinkSpeed = 1000000000;
            break;
        }
    } else {
        LinkState->MediaConnectState = MediaConnectStateDisconnected;
        LinkState->MediaDuplexState = MediaDuplexStateUnknown;
        LinkState->XmitLinkSpeed = 0;
        LinkState->RcvLinkSpeed = 0;
    }

    // Если включены игровые оптимизации, добавляем дополнительную информацию
    if (deviceContext->TrafficPrioritizationEnabled) {
        LinkState->PauseFunctions = NetAdapterPauseFunctionsFlagReceive | NetAdapterPauseFunctionsFlagSend;
    }
}

// Обработчик создания очереди передачи
NTSTATUS
I219vEvtCreateTxQueue(
    _In_ NETADAPTER Adapter,
    _In_ NET_PACKET_QUEUE_CONFIG Configuration,
    _Out_ PNETPACKETQUEUE* TxQueue
    )
{
    NTSTATUS status;
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));
    WDF_OBJECT_ATTRIBUTES txQueueAttributes;
    NETPACKETQUEUE txQueue;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Creating TX queue");

    // Инициализация атрибутов очереди передачи
    WDF_OBJECT_ATTRIBUTES_INIT(&txQueueAttributes);
    txQueueAttributes.ParentObject = Adapter;

    // Создание очереди передачи
    status = NetTxQueueCreate(
        Configuration,
        &txQueueAttributes,
        &txQueue);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, "NetTxQueueCreate failed: %!STATUS!", status);
        return status;
    }

    // Если включена приоритизация трафика, настраиваем очередь для поддержки приоритетов
    if (deviceContext->TrafficPrioritizationEnabled) {
        // В реальной реализации здесь бы выполнялась настройка очереди для поддержки приоритетов
        // Например, создание нескольких физических очередей с разными приоритетами
    }

    *TxQueue = txQueue;
    return STATUS_SUCCESS;
}

// Обработчик создания очереди приема
NTSTATUS
I219vEvtCreateRxQueue(
    _In_ NETADAPTER Adapter,
    _In_ NET_PACKET_QUEUE_CONFIG Configuration,
    _Out_ PNETPACKETQUEUE* RxQueue
    )
{
    NTSTATUS status;
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));
    WDF_OBJECT_ATTRIBUTES rxQueueAttributes;
    NETPACKETQUEUE rxQueue;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Creating RX queue");

    // Инициализация атрибутов очереди приема
    WDF_OBJECT_ATTRIBUTES_INIT(&rxQueueAttributes);
    rxQueueAttributes.ParentObject = Adapter;

    // Создание очереди приема
    status = NetRxQueueCreate(
        Configuration,
        &rxQueueAttributes,
        &rxQueue);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, "NetRxQueueCreate failed: %!STATUS!", status);
        return status;
    }

    // Если включена приоритизация трафика, настраиваем очередь для поддержки приоритетов
    if (deviceContext->TrafficPrioritizationEnabled) {
        // В реальной реализации здесь бы выполнялась настройка очереди для поддержки приоритетов
        // Например, создание нескольких физических очередей с разными приоритетами
    }

    *RxQueue = rxQueue;
    return STATUS_SUCCESS;
}

// Обработчик паузы адаптера
NTSTATUS
I219vEvtAdapterPause(
    _In_ NETADAPTER Adapter,
    _In_ NETADAPTER_PAUSE_PARAMETERS* PauseParameters
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    UNREFERENCED_PARAMETER(PauseParameters);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Adapter pause");

    // Сохранение статистики производительности перед паузой
    if (deviceContext->TrafficPrioritizationEnabled) {
        // В реальной реализации здесь бы выполнялось сохранение статистики
    }

    return STATUS_SUCCESS;
}

// Обработчик перезапуска адаптера
NTSTATUS
I219vEvtAdapterRestart(
    _In_ NETADAPTER Adapter
    )
{
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(NetAdapterGetDevice(Adapter));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Adapter restart");

    // Если требуется перезапуск адаптера из-за изменения настроек
    if (deviceContext->NeedResetAdapter) {
        // В реальной реализации здесь бы выполнялся перезапуск адаптера
        deviceContext->NeedResetAdapter = FALSE;
    }

    // Применение игрового профиля после перезапуска
    if (deviceContext->TrafficPrioritizationEnabled || 
        deviceContext->LatencyReductionEnabled || 
        deviceContext->BandwidthControlEnabled) {
        I219vApplyGamingProfile(deviceContext, &deviceContext->GamingProfile);
    }

    return STATUS_SUCCESS;
}
