/*++

Copyright (c) 2025 Manus AI

Module Name:

    NetAdapterConfig.c

Abstract:

    Реализация функций для настройки и конфигурации NetAdapter
    Содержит обработчики событий и настройку возможностей адаптера

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

// Обработчик создания NetAdapter
NTSTATUS
I219vCreateNetAdapter(
    _In_ WDFDEVICE Device
    )
{
    NTSTATUS status;
    PI219V_DEVICE_CONTEXT deviceContext;
    NET_ADAPTER_CONFIG netAdapterConfig;
    NETADAPTER netAdapter;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Creating NetAdapter");

    deviceContext = I219vGetDeviceContext(Device);
    deviceContext->Device = Device;

    // Инициализация конфигурации NetAdapter
    NET_ADAPTER_CONFIG_INIT(
        &netAdapterConfig,
        I219vEvtAdapterCreateRxQueue,
        I219vEvtAdapterCreateTxQueue
    );

    // Настройка обработчиков событий
    netAdapterConfig.EvtAdapterSetCapabilities = I219vEvtAdapterSetCapabilities;
    netAdapterConfig.EvtAdapterStart = I219vEvtAdapterStart;
    netAdapterConfig.EvtAdapterStop = I219vEvtAdapterStop;
    netAdapterConfig.EvtAdapterPause = I219vEvtAdapterPause;
    netAdapterConfig.EvtAdapterRestart = I219vEvtAdapterRestart;
    netAdapterConfig.EvtAdapterCreateTxQueue = I219vEvtAdapterCreateTxQueue;
    netAdapterConfig.EvtAdapterCreateRxQueue = I219vEvtAdapterCreateRxQueue;

    // Создание объекта NetAdapter
    status = NetAdapterCreate(Device, &netAdapterConfig, &netAdapter);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterCreate failed %!STATUS!", status);
        return status;
    }

    // Сохранение объекта NetAdapter в контексте устройства
    deviceContext->NetAdapter = netAdapter;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter created successfully");
    return STATUS_SUCCESS;
}

// Обработчик установки возможностей адаптера
VOID
I219vEvtAdapterSetCapabilities(
    _In_ NETADAPTER NetAdapter
    )
{
    NTSTATUS status;
    PI219V_DEVICE_CONTEXT deviceContext;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    NET_ADAPTER_LINK_LAYER_ADDRESS linkLayerAddress;
    NET_ADAPTER_POWER_CAPABILITIES powerCapabilities;
    NET_ADAPTER_DMA_CAPABILITIES dmaCapabilities;
    NET_ADAPTER_RECEIVE_CAPABILITIES receiveCapabilities;
    NET_ADAPTER_RECEIVE_FILTER_CAPABILITIES receiveFilterCapabilities;
    NET_ADAPTER_OFFLOAD_CAPABILITIES offloadCapabilities;
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Setting NetAdapter capabilities");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Настройка возможностей канального уровня
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(
        &linkLayerCapabilities, 
        I219V_MAX_LINK_SPEED, 
        I219V_MIN_LINK_SPEED
    );
    
    // Добавление поддерживаемых скоростей соединения
    NetAdapterLinkLayerCapabilitiesAddLinkSpeed(&linkLayerCapabilities, NDIS_LINK_SPEED_1000MBPS);
    NetAdapterLinkLayerCapabilitiesAddLinkSpeed(&linkLayerCapabilities, NDIS_LINK_SPEED_100MBPS);
    NetAdapterLinkLayerCapabilitiesAddLinkSpeed(&linkLayerCapabilities, NDIS_LINK_SPEED_10MBPS);
    
    // Установка режимов дуплекса
    NetAdapterLinkLayerCapabilitiesAddMediaDuplexState(&linkLayerCapabilities, MediaDuplexStateFull);
    NetAdapterLinkLayerCapabilitiesAddMediaDuplexState(&linkLayerCapabilities, MediaDuplexStateHalf);
    
    // Установка возможностей канального уровня
    status = NetAdapterSetLinkLayerCapabilities(NetAdapter, &linkLayerCapabilities);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterSetLinkLayerCapabilities failed %!STATUS!", status);
        return;
    }

    // Установка MAC-адреса
    NET_ADAPTER_LINK_LAYER_ADDRESS_INIT(
        &linkLayerAddress, 
        deviceContext->MacAddress, 
        sizeof(deviceContext->MacAddress)
    );
    
    status = NetAdapterSetCurrentLinkLayerAddress(NetAdapter, &linkLayerAddress);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterSetCurrentLinkLayerAddress failed %!STATUS!", status);
        return;
    }

    // Настройка возможностей управления питанием
    NET_ADAPTER_POWER_CAPABILITIES_INIT(&powerCapabilities);
    
    // Поддержка Wake-on-LAN
    powerCapabilities.SupportedWakePatterns = NET_ADAPTER_WAKE_PATTERN_MAGIC_PACKET;
    
    status = NetAdapterSetPowerCapabilities(NetAdapter, &powerCapabilities);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterSetPowerCapabilities failed %!STATUS!", status);
        return;
    }

    // Настройка возможностей DMA
    NET_ADAPTER_DMA_CAPABILITIES_INIT(&dmaCapabilities);
    
    // Установка возможностей DMA
    status = NetAdapterSetDataPathCapabilities(NetAdapter, &dmaCapabilities);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterSetDataPathCapabilities failed %!STATUS!", status);
        return;
    }

    // Настройка возможностей приема
    NET_ADAPTER_RECEIVE_CAPABILITIES_INIT(&receiveCapabilities);
    
    // Установка максимального размера пакета
    receiveCapabilities.MaximumFrameSize = I219V_MAX_PACKET_SIZE;
    
    status = NetAdapterSetReceiveCapabilities(NetAdapter, &receiveCapabilities);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterSetReceiveCapabilities failed %!STATUS!", status);
        return;
    }

    // Настройка возможностей фильтрации приема
    NET_ADAPTER_RECEIVE_FILTER_CAPABILITIES_INIT(&receiveFilterCapabilities);
    
    // Поддержка фильтрации по MAC-адресу
    receiveFilterCapabilities.SupportedPacketFilters = 
        NET_PACKET_FILTER_TYPE_DIRECTED |
        NET_PACKET_FILTER_TYPE_MULTICAST |
        NET_PACKET_FILTER_TYPE_BROADCAST |
        NET_PACKET_FILTER_TYPE_PROMISCUOUS |
        NET_PACKET_FILTER_TYPE_ALL_MULTICAST;
    
    // Максимальное количество мультикаст-адресов
    receiveFilterCapabilities.MaximumMulticastAddresses = 16;
    
    status = NetAdapterSetReceiveFilterCapabilities(NetAdapter, &receiveFilterCapabilities);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterSetReceiveFilterCapabilities failed %!STATUS!", status);
        return;
    }

    // Настройка возможностей аппаратных оффлоадов
    NET_ADAPTER_OFFLOAD_CAPABILITIES_INIT(&offloadCapabilities);
    
    // Поддержка оффлоада контрольной суммы
    offloadCapabilities.Checksum.IPv4 = NET_ADAPTER_OFFLOAD_SUPPORTED;
    offloadCapabilities.Checksum.Tcp = NET_ADAPTER_OFFLOAD_SUPPORTED;
    offloadCapabilities.Checksum.Udp = NET_ADAPTER_OFFLOAD_SUPPORTED;
    
    // Поддержка оффлоада сегментации TCP
    offloadCapabilities.LargeSendOffload.IPv4 = NET_ADAPTER_OFFLOAD_SUPPORTED;
    
    status = NetAdapterOffloadSetCapabilities(NetAdapter, &offloadCapabilities);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "NetAdapterOffloadSetCapabilities failed %!STATUS!", status);
        return;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter capabilities set successfully");
}

// Обработчик запуска адаптера
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

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Starting NetAdapter");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Инициализация путей данных
    status = I219vInitializeDatapath(deviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "I219vInitializeDatapath failed %!STATUS!", status);
        return;
    }

    // Инициализация прерываний
    status = I219vInitializeInterrupt(device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_ADAPTER, 
                  "I219vInitializeInterrupt failed %!STATUS!", status);
        I219vCleanupDatapath(deviceContext);
        return;
    }

    // Включение устройства
    I219vEnableDevice(deviceContext);

    // Проверка состояния соединения
    statusReg = I219vReadRegister(deviceContext, I219V_REG_STATUS);
    
    if (statusReg & I219V_STATUS_LU) {
        // Соединение установлено
        NET_ADAPTER_LINK_STATE_INIT(
            &linkState,
            NDIS_LINK_SPEED_1000MBPS,
            MediaConnectStateConnected,
            MediaDuplexStateFull,
            NetAdapterPauseFunctionTypeUnsupported,
            NetAdapterAutoNegotiationFlagXmitLinkSpeed |
            NetAdapterAutoNegotiationFlagRcvLinkSpeed |
            NetAdapterAutoNegotiationFlagDuplexMode
        );
    } else {
        // Соединение отсутствует
        NET_ADAPTER_LINK_STATE_INIT_DISCONNECTED(&linkState);
    }
    
    // Уведомление об изменении состояния соединения
    NetAdapterSetLinkState(NetAdapter, &linkState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter started successfully");
}

// Обработчик остановки адаптера
VOID
I219vEvtAdapterStop(
    _In_ NETADAPTER NetAdapter
    )
{
    PI219V_DEVICE_CONTEXT deviceContext;
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Stopping NetAdapter");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Отключение устройства
    I219vDisableDevice(deviceContext);

    // Очистка ресурсов путей данных
    I219vCleanupDatapath(deviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter stopped");
}

// Обработчик приостановки адаптера
VOID
I219vEvtAdapterPause(
    _In_ NETADAPTER NetAdapter
    )
{
    PI219V_DEVICE_CONTEXT deviceContext;
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Pausing NetAdapter");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Приостановка устройства
    I219vPauseDevice(deviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter paused");
}

// Обработчик возобновления работы адаптера
VOID
I219vEvtAdapterRestart(
    _In_ NETADAPTER NetAdapter
    )
{
    PI219V_DEVICE_CONTEXT deviceContext;
    WDFDEVICE device;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "Restarting NetAdapter");

    device = NetAdapterGetWdfDevice(NetAdapter);
    deviceContext = I219vGetDeviceContext(device);

    // Возобновление работы устройства
    I219vRestartDevice(deviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ADAPTER, "NetAdapter restarted");
}
