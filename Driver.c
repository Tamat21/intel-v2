/*++

Copyright (c) 2025 Manus AI

Module Name:

    Driver.c

Abstract:

    Реализация точки входа драйвера Intel i219-v с поддержкой игровых оптимизаций.
    Включает инициализацию драйвера и регистрацию функций обратного вызова.
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
#include "i219v_gaming.h"
#include "Trace.h"

// Версия драйвера
#define I219V_DRIVER_VERSION_MAJOR 1
#define I219V_DRIVER_VERSION_MINOR 0
#define I219V_DRIVER_VERSION_BUILD 0
#define I219V_DRIVER_VERSION_REVISION 0

// Информация о драйвере
PCWSTR I219vDriverName = L"Intel i219-v Gaming Driver";
PCWSTR I219vDriverVersion = L"1.0.0.0";
PCWSTR I219vDriverDescription = L"Intel i219-v Gaming Driver с оптимизациями Killer Performance";

// Глобальные переменные
WDFDRIVER I219vDriver = NULL;

// Функция входа драйвера
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;
    WDFDRIVER driver;

    // Инициализация трассировки
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Driver Entry");

    // Инициализация конфигурации драйвера
    WDF_DRIVER_CONFIG_INIT(&config, I219vEvtDeviceAdd);
    config.DriverPoolTag = 'v912';

    // Создание объекта драйвера
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        &driver);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed: %!STATUS!", status);
        return status;
    }

    // Сохранение дескриптора драйвера
    I219vDriver = driver;

    // Регистрация клиента NetAdapterCx
    status = NetAdapterCxRegisterClient(driver);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "NetAdapterCxRegisterClient failed: %!STATUS!", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Driver Entry completed successfully");
    return status;
}

// Функция добавления устройства
NTSTATUS
I219vEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    NETADAPTER_INIT* adapterInit = NULL;
    NETADAPTER adapter = NULL;
    PI219V_DEVICE_CONTEXT deviceContext = NULL;

    UNREFERENCED_PARAMETER(Driver);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Device Add");

    // Настройка атрибутов устройства
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, I219V_DEVICE_CONTEXT);
    deviceAttributes.EvtCleanupCallback = I219vEvtDeviceContextCleanup;

    // Создание объекта устройства
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDeviceCreate failed: %!STATUS!", status);
        goto Exit;
    }

    // Получение контекста устройства
    deviceContext = I219vGetDeviceContext(device);
    RtlZeroMemory(deviceContext, sizeof(I219V_DEVICE_CONTEXT));
    deviceContext->Device = device;

    // Инициализация устройства
    status = I219vInitializeDevice(deviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "I219vInitializeDevice failed: %!STATUS!", status);
        goto Exit;
    }

    // Создание инициализатора адаптера
    adapterInit = NetAdapterInitAllocate(device);
    if (adapterInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "NetAdapterInitAllocate failed");
        goto Exit;
    }

    // Настройка обратных вызовов пути данных
    NET_ADAPTER_DATAPATH_CALLBACKS_INIT(&datapathCallbacks, I219vEvtCreateTxQueue, I219vEvtCreateRxQueue);
    NetAdapterInitSetDatapathCallbacks(adapterInit, &datapathCallbacks);

    // Регистрация обратных вызовов адаптера
    status = I219vRegisterAdapterCallbacks(adapterInit, deviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "I219vRegisterAdapterCallbacks failed: %!STATUS!", status);
        goto Exit;
    }

    // Создание объекта адаптера
    status = NetAdapterCreate(adapterInit, WDF_NO_OBJECT_ATTRIBUTES, &adapter);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "NetAdapterCreate failed: %!STATUS!", status);
        goto Exit;
    }

    // Сохранение дескриптора адаптера
    deviceContext->NetAdapter = adapter;

    // Инициализация игровых функций
    status = I219vInitializeGamingFeatures(deviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "I219vInitializeGamingFeatures failed: %!STATUS!", status);
        goto Exit;
    }

    // Запуск адаптера
    status = NetAdapterStart(adapter);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "NetAdapterStart failed: %!STATUS!", status);
        goto Exit;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Device Add completed successfully");

Exit:
    if (adapterInit != NULL) {
        NetAdapterInitFree(adapterInit);
    }

    return status;
}

// Функция очистки контекста устройства
VOID
I219vEvtDeviceContextCleanup(
    _In_ WDFOBJECT Object
    )
{
    WDFDEVICE device = (WDFDEVICE)Object;
    PI219V_DEVICE_CONTEXT deviceContext = I219vGetDeviceContext(device);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Device Context Cleanup");

    // Освобождение ресурсов устройства
    if (deviceContext->IoBase != NULL) {
        MmUnmapIoSpace(deviceContext->IoBase, deviceContext->IoSize);
        deviceContext->IoBase = NULL;
    }
}

// Функция регистрации обратных вызовов адаптера
NTSTATUS
I219vRegisterAdapterCallbacks(
    _In_ NETADAPTER_INIT* AdapterInit,
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    NET_ADAPTER_LINK_LAYER_ADDRESS linkLayerAddress;
    NET_ADAPTER_POWER_CAPABILITIES powerCapabilities;
    NET_ADAPTER_DMA_CAPABILITIES dmaCapabilities;
    NET_ADAPTER_RECEIVE_CAPABILITIES receiveCapabilities;
    NET_ADAPTER_OFFLOAD_CAPABILITIES offloadCapabilities;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Registering adapter callbacks");

    // Установка обратных вызовов адаптера
    NetAdapterInitSetLinkLayerCapabilitiesCallback(AdapterInit, I219vEvtAdapterSetLinkLayerCapabilities, DeviceContext);
    NetAdapterInitSetLinkLayerAddressCallback(AdapterInit, I219vEvtAdapterSetLinkLayerAddress, DeviceContext);
    NetAdapterInitSetPowerCapabilitiesCallback(AdapterInit, I219vEvtAdapterSetPowerCapabilities, DeviceContext);
    NetAdapterInitSetReceiveCapabilitiesCallback(AdapterInit, I219vEvtAdapterSetReceiveCapabilities, DeviceContext);
    NetAdapterInitSetOffloadCapabilitiesCallback(AdapterInit, I219vEvtAdapterSetOffloadCapabilities, DeviceContext);
    NetAdapterInitSetCurrentLinkStateCallback(AdapterInit, I219vEvtAdapterSetCurrentLinkState, DeviceContext);
    NetAdapterInitSetPermanentLinkLayerAddressCallback(AdapterInit, I219vEvtAdapterSetPermanentLinkLayerAddress, DeviceContext);
    NetAdapterInitSetDmaCapabilitiesCallback(AdapterInit, I219vEvtAdapterSetDmaCapabilities, DeviceContext);

    // Установка обработчиков событий адаптера
    status = NetAdapterInitSetPauseCallback(AdapterInit, I219vEvtAdapterPause, DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "NetAdapterInitSetPauseCallback failed: %!STATUS!", status);
        return status;
    }

    status = NetAdapterInitSetRestartCallback(AdapterInit, I219vEvtAdapterRestart, DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "NetAdapterInitSetRestartCallback failed: %!STATUS!", status);
        return status;
    }

    // Инициализация возможностей адаптера
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(&linkLayerCapabilities, NdisMedium802_3, 1500);
    NET_ADAPTER_LINK_LAYER_ADDRESS_INIT(&linkLayerAddress, DeviceContext->MacAddress, 6);
    NET_ADAPTER_POWER_CAPABILITIES_INIT(&powerCapabilities);
    NET_ADAPTER_DMA_CAPABILITIES_INIT(&dmaCapabilities);
    NET_ADAPTER_RECEIVE_CAPABILITIES_INIT(&receiveCapabilities);
    NET_ADAPTER_OFFLOAD_CAPABILITIES_INIT(&offloadCapabilities);

    // Настройка возможностей адаптера
    linkLayerCapabilities.MaximumMulticastListSize = 16;
    linkLayerCapabilities.SupportedPacketFilters = 
        NetPacketFilterFlagDirected |
        NetPacketFilterFlagMulticast |
        NetPacketFilterFlagBroadcast |
        NetPacketFilterFlagPromiscuous |
        NetPacketFilterFlagAllMulticast;

    powerCapabilities.SupportedWakePatterns =
        NetWakePatternFlagBitmapPattern |
        NetWakePatternFlagMagicPacket;

    // Настройка возможностей DMA
    dmaCapabilities.MaximumPhysicalAddress.QuadPart = MAXULONG64;
    dmaCapabilities.PreferredNode = MM_ANY_NODE_OK;

    // Настройка возможностей приема
    receiveCapabilities.MaximumReceiveQueueCount = 1;
    receiveCapabilities.MaximumReceiveQueueGroupCount = 1;

    // Настройка возможностей оффлоадов
    offloadCapabilities.SupportedChecksumOffloads =
        NetAdapterOffloadChecksumFlagIPv4Transmit |
        NetAdapterOffloadChecksumFlagTcpTransmit |
        NetAdapterOffloadChecksumFlagUdpTransmit |
        NetAdapterOffloadChecksumFlagIPv4Receive |
        NetAdapterOffloadChecksumFlagTcpReceive |
        NetAdapterOffloadChecksumFlagUdpReceive;

    offloadCapabilities.SupportedLsoOffloads =
        NetAdapterOffloadLsoFlagIPv4 |
        NetAdapterOffloadLsoFlagIPv6;

    // Установка возможностей адаптера
    NetAdapterSetLinkLayerCapabilities(AdapterInit, &linkLayerCapabilities);
    NetAdapterSetLinkLayerAddress(AdapterInit, &linkLayerAddress);
    NetAdapterSetPowerCapabilities(AdapterInit, &powerCapabilities);
    NetAdapterSetDmaCapabilities(AdapterInit, &dmaCapabilities);
    NetAdapterSetReceiveCapabilities(AdapterInit, &receiveCapabilities);
    NetAdapterSetOffloadCapabilities(AdapterInit, &offloadCapabilities);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Adapter callbacks registered successfully");
    return STATUS_SUCCESS;
}

// Функция получения контекста устройства
PI219V_DEVICE_CONTEXT
I219vGetDeviceContext(
    _In_ WDFDEVICE Device
    )
{
    return (PI219V_DEVICE_CONTEXT)WdfObjectGet_I219V_DEVICE_CONTEXT(Device);
}
