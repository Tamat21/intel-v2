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

// Define placeholders used in I219vEvtAdapterSetCapabilities
// These should ideally be in a central hardware definition header.
#define HW_MAX_MULTICAST_LIST_SIZE 32
#define DEFAULT_MTU_SIZE 1500
#define MAX_TX_QUEUES 1
#define MAX_RX_QUEUES 2 // For RSS
#define MAX_ETHERNET_HEADER_SIZE 22 // Max Ethernet header size (including VLAN)
#define RSS_INDIRECTION_TABLE_SIZE 128 // Common size for RSS indirection table

// Обработчик установки текущего состояния соединения
// This function remains as it's a specific callback for link state changes.
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
    // This function is actually implemented in Queue.c and registered there.
    // The declaration in Device.h and this stub in Adapter.c can be confusing.
    // For now, leaving this stub as is, assuming the real one in Queue.c is used.
    // Ideally, this duplicate stub should be removed if Queue.c's version is authoritative.
    UNREFERENCED_PARAMETER(Configuration);
    UNREFERENCED_PARAMETER(TxQueue);
    TraceEvents(TRACE_LEVEL_WARNING, TRACE_ADAPTER, "I219vEvtCreateTxQueue in Adapter.c called - should be Queue.c's version");
    return STATUS_NOT_IMPLEMENTED;
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
    // This function is actually implemented in Queue.c and registered there.
    // The declaration in Device.h and this stub in Adapter.c can be confusing.
    // For now, leaving this stub as is, assuming the real one in Queue.c is used.
    // Ideally, this duplicate stub should be removed if Queue.c's version is authoritative.
    UNREFERENCED_PARAMETER(Configuration);
    UNREFERENCED_PARAMETER(RxQueue);
    TraceEvents(TRACE_LEVEL_WARNING, TRACE_ADAPTER, "I219vEvtCreateRxQueue in Adapter.c called - should be Queue.c's version");
    return STATUS_NOT_IMPLEMENTED;
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

// Обработчик установки возможностей адаптера (moved from NetAdapterConfig.c)
VOID
I219vEvtAdapterSetCapabilities(
    _In_ NETADAPTER NetAdapter
    )
{
    NTSTATUS status;
    PI219V_DEVICE_CONTEXT deviceContext;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    // NET_ADAPTER_LINK_LAYER_ADDRESS linkLayerAddress; // Not set here, set by its own callback
    NET_ADAPTER_POWER_CAPABILITIES powerCapabilities;
    NET_ADAPTER_DATA_PATH_CAPABILITIES dataPathCapabilities; // Corrected from NET_ADAPTER_DMA_CAPABILITIES
    NET_ADAPTER_RECEIVE_CAPABILITIES receiveCapabilities;
    NET_ADAPTER_RECEIVE_FILTER_CAPABILITIES receiveFilterCapabilities;
    NET_ADAPTER_TX_CAPABILITIES txCapabilities; 
    NET_ADAPTER_RX_CAPABILITIES rxOffloadCapabilities; 
    NET_ADAPTER_RECEIVE_SCALING_CAPABILITIES rssCapabilities; // For RSS
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting NetAdapter capabilities (consolidated)");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Link Layer Capabilities
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(
        &linkLayerCapabilities,
        I219V_MAX_LINK_SPEED,
        I219V_MAX_LINK_SPEED
    );
    linkLayerCapabilities.SupportedPacketFilters =
        NetPacketFilterFlagDirected |
        NetPacketFilterFlagMulticast |
        NetPacketFilterFlagBroadcast |
        NetPacketFilterFlagPromiscuous |
        NetPacketFilterFlagAllMulticast;
    linkLayerCapabilities.MaximumMulticastListSize = HW_MAX_MULTICAST_LIST_SIZE; // Define this constant based on HW
    // MTU is read from deviceContext, assuming it's initialized from INF or a default.
    linkLayerCapabilities.MtuSize = (deviceContext->MTU > 0) ? deviceContext->MTU : DEFAULT_MTU_SIZE; // Define DEFAULT_MTU_SIZE
    NetAdapterSetLinkLayerCapabilities(NetAdapter, &linkLayerCapabilities);

    // MAC Address: Set by I219vEvtAdapterSetLinkLayerAddress and I219vEvtAdapterSetPermanentLinkLayerAddress callbacks.

    // Power Capabilities
    NET_ADAPTER_POWER_CAPABILITIES_INIT(&powerCapabilities);
    powerCapabilities.SupportedWakePatterns =
        NetWakePatternFlagBitmapPattern |
        NetWakePatternFlagMagicPacket;
    WdfSpinLockAcquire(deviceContext->GamingSettingsLock);
    if (deviceContext->SmartPowerManagementEnabled) {
        powerCapabilities.SupportedWakePatterns |= NetWakePatternFlagWakeOnMediaDisconnect;
        // Note: NetProtocolOffloadFlagArpNs is for NDIS_PM_PROTOCOL_OFFLOAD, not directly NET_ADAPTER_POWER_CAPABILITIES
        // This might need to be set via NDIS_PM_PARAMETERS if advanced PM offloads are used.
        // For NET_ADAPTER_POWER_CAPABILITIES, focus on wake patterns.
    }
    WdfSpinLockRelease(deviceContext->GamingSettingsLock);
    NetAdapterSetPowerCapabilities(NetAdapter, &powerCapabilities);

    // Data Path Capabilities (replaces DMA capabilities for NetAdapterCx)
    NET_ADAPTER_DATA_PATH_CAPABILITIES_INIT(&dataPathCapabilities);
    dataPathCapabilities.MaximumPhysicalAddress.QuadPart = MAXULONG64; // Typical for 64-bit DMA
    dataPathCapabilities.PreferredNode = MM_ANY_NODE_OK;
    // Specify Tx & Rx capabilities
    dataPathCapabilities.TxCapabilities.MaximumNumberOfQueues = MAX_TX_QUEUES; // Define this (e.g., 1)
    dataPathCapabilities.RxCapabilities.MaximumNumberOfQueues = MAX_RX_QUEUES; // Define this (e.g., 1)
    // Other fields like SGE, alignment requirements would be set here.
    NetAdapterSetDataPathCapabilities(NetAdapter, &dataPathCapabilities);

    // Receive Capabilities (General)
    NET_ADAPTER_RECEIVE_CAPABILITIES_INIT(&receiveCapabilities);
    receiveCapabilities.MaximumFrameSize = linkLayerCapabilities.MtuSize + MAX_ETHERNET_HEADER_SIZE; // e.g. 14/18/22
    receiveCapabilities.MaximumReceiveQueues = MAX_RX_QUEUES; // Define this
    NetAdapterSetReceiveCapabilities(NetAdapter, &receiveCapabilities);

    // Receive Filter Capabilities
    NET_ADAPTER_RECEIVE_FILTER_CAPABILITIES_INIT(&receiveFilterCapabilities);
    receiveFilterCapabilities.SupportedPacketFilters = linkLayerCapabilities.SupportedPacketFilters;
    receiveFilterCapabilities.MaximumMulticastAddresses = linkLayerCapabilities.MaximumMulticastListSize;
    NetAdapterSetReceiveFilterCapabilities(NetAdapter, &receiveFilterCapabilities);

    // Offload Capabilities
    // Transmit Offloads
    NET_ADAPTER_TX_CAPABILITIES_INIT_FOR_CHECKSUM_OFFLOAD(&txCapabilities,
                                नेटAdapterOffloadLayer3FlagIPv4NoOptions, // Layer3 flags
                                NetAdapterOffloadLayer4FlagTcpNoOptions | NetAdapterOffloadLayer4FlagUdpNoOptions // Layer4 flags
                                );
    // Example: Enable LSOv1/LSOv2 if supported
    // NET_ADAPTER_LARGE_SEND_OFFLOAD_CAPABILITIES lsoCaps;
    // NET_ADAPTER_LARGE_SEND_OFFLOAD_CAPABILITIES_INIT(&lsoCaps, NetAdapterOffloadLayer3FlagIPv4NoOptions, ...);
    // NetAdapterOffloadSetTxLargeSendOffloadCapabilities(NetAdapter, &lsoCaps);
    NetAdapterOffloadSetTxChecksumCapabilities(NetAdapter, &txCapabilities);


    // Receive Offloads
    NET_ADAPTER_RX_CAPABILITIES_INIT_FOR_CHECKSUM_OFFLOAD(&rxOffloadCapabilities,
                                NetAdapterOffloadLayer3FlagIPv4NoOptions,
                                NetAdapterOffloadLayer4FlagTcpNoOptions | NetAdapterOffloadLayer4FlagUdpNoOptions
                                );
    NetAdapterOffloadSetRxChecksumCapabilities(NetAdapter, &rxOffloadCapabilities);

    // Receive Side Scaling (RSS) Capabilities
    // I219-V supports 2 queues.
    NET_ADAPTER_RECEIVE_SCALING_CAPABILITIES_INIT(
        &rssCapabilities,
        MAX_RX_QUEUES, // Number of Queues
        NetAdapterReceiveScalingIndirectionTableSize128 // Indirection table size
    );
    // Specify hash functions - typically TCP/UDP over IPv4/IPv6
    rssCapabilities.SupportedHashTypes = 
        NetAdapterReceiveScalingHashTypeToeplitz | 
        NetAdapterReceiveScalingHashTypeNone; // None might be required if disabling RSS dynamically
    
    rssCapabilities.SupportedProtocolTypes = 
        NetAdapterReceiveScalingProtocolTypeIPv4 |
        NetAdapterReceiveScalingProtocolTypeIPv6 |
        NetAdapterReceiveScalingProtocolTypeIPv4Options | // If options are processed
        NetAdapterReceiveScalingProtocolTypeIPv6Extensions | // If extensions are processed
        NetAdapterReceiveScalingProtocolTypeTcp |
        NetAdapterReceiveScalingProtocolTypeUdp;

    // Unhashed target (fallback if hash computation fails or for non-RSSable packets)
    // Typically set to 0, or an unhashed queue if one exists.
    rssCapabilities.UnhashedTarget = 0; 
    rssCapabilities.Flags = NetAdapterReceiveScalingFlagHashInformation | // Driver provides hash info
                            NetAdapterReceiveScalingFlagIndirectionTableUpdates; // Driver handles updates

    NetAdapterOffloadSetReceiveSideScalingCapabilities(NetAdapter, &rssCapabilities);


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter capabilities set (consolidated)");
}

// Обработчик запуска адаптера (moved from NetAdapterConfig.c)
VOID
I219vEvtAdapterStart(
    _In_ NETADAPTER NetAdapter
    )
{
    NTSTATUS status;
    PI219V_DEVICE_CONTEXT deviceContext;
    NET_ADAPTER_LINK_STATE linkState;
    UINT32 statusReg;
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Starting NetAdapter (consolidated)");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Инициализация путей данных
    status = I219vInitializeDatapath(deviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER,
                  "I219vInitializeDatapath failed %!STATUS!", status);
        // Consider how to handle this failure. Typically, Start should not fail,
        // or if it does, the adapter won't start.
        return;
    }

    // Инициализация прерываний (assuming I219vInitializeInterrupt is defined and available)
    // This was in NetAdapterConfig.c's Start. It implies interrupt object needs to be valid.
    // The interrupt object is typically created in EvtDevicePrepareHardware or EvtDeviceD0Entry.
    // Let's assume it's created and just needs enabling or connecting here if applicable.
    // For now, ensure the function exists or remove if init is done elsewhere.
    // status = I219vInitializeInterrupt(device); // This needs to be available or removed.
    // if (!NT_SUCCESS(status)) {
    //     TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER,
    //               "I219vInitializeInterrupt failed %!STATUS!", status);
    //     I219vCleanupDatapath(deviceContext); // Clean up what was done
    //     return;
    // }


    // Включение устройства (hardware enable)
    I219vEnableDevice(deviceContext); // Assumes this function correctly enables HW for operation

    // Проверка состояния соединения
    // This logic is similar to I219vEvtAdapterSetCurrentLinkState.
    // SetCurrentLinkState is a callback. Here, we might want to report the *initial* state.
    statusReg = I219vReadRegister(deviceContext, I219V_REG_STATUS);

    if (statusReg & I219V_STATUS_LU) {
        NET_ADAPTER_LINK_STATE_INIT(
            &linkState,
            NDIS_LINK_SPEED_1000MBPS, // Example, should be determined from HW
            MediaConnectStateConnected,
            MediaDuplexStateFull,     // Example, should be determined from HW
            NetAdapterPauseFunctionTypeUnsupported, // Example
            NetAdapterAutoNegotiationFlagXmitLinkSpeed |
            NetAdapterAutoNegotiationFlagRcvLinkSpeed |
            NetAdapterAutoNegotiationFlagDuplexMode
        );
    } else {
        NET_ADAPTER_LINK_STATE_INIT_DISCONNECTED(&linkState);
    }
    NetAdapterSetLinkState(NetAdapter, &linkState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter started successfully (consolidated)");
}

// Обработчик остановки адаптера (moved from NetAdapterConfig.c)
VOID
I219vEvtAdapterStop(
    _In_ NETADAPTER NetAdapter
    )
{
    PI219V_DEVICE_CONTEXT deviceContext;
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Stopping NetAdapter (consolidated)");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Отключение устройства (hardware disable)
    I219vDisableDevice(deviceContext); // Assumes this function correctly disables HW

    // Очистка ресурсов путей данных
    I219vCleanupDatapath(deviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter stopped (consolidated)");
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

    NTSTATUS status = STATUS_SUCCESS; // Ensure status is initialized

    // Применение игрового профиля после перезапуска
    WdfSpinLockAcquire(deviceContext->GamingSettingsLock);
    BOOLEAN applyGamingProfile = deviceContext->TrafficPrioritizationEnabled || 
                                 deviceContext->LatencyReductionEnabled || 
                                 deviceContext->BandwidthControlEnabled;
    I219V_GAMING_PROFILE currentGamingProfile = deviceContext->GamingProfile; // Copy profile under lock
    WdfSpinLockRelease(deviceContext->GamingSettingsLock);

    if (applyGamingProfile) {
        status = I219vApplyGamingProfile(deviceContext, &currentGamingProfile);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                        "I219vApplyGamingProfile failed in EvtAdapterRestart: %!STATUS!", status);
            // Depending on policy, this might be a non-fatal error for restart.
        }
    }

    return STATUS_SUCCESS; // EvtAdapterRestart itself usually returns STATUS_SUCCESS unless a catastrophic failure.
}
