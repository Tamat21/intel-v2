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

    // Инициализация блокировки для игровых настроек
    WDF_OBJECT_ATTRIBUTES lockAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);
    lockAttributes.ParentObject = device; // Связываем блокировку с устройством для автоматической очистки

    status = WdfSpinLockCreate(&lockAttributes, &deviceContext->GamingSettingsLock);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfSpinLockCreate failed for GamingSettingsLock: %!STATUS!", status);
        goto Exit;
    }

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

    // Установка обратных вызовов жизненного цикла адаптера
    // Главный обработчик установки возможностей (теперь в Adapter.c)
    NetAdapterInitSetAdapterSetCapabilitiesCallback(AdapterInit, I219vEvtAdapterSetCapabilities);
    // Обработчики Start/Stop (теперь в Adapter.c)
    NetAdapterInitSetAdapterStartCallback(AdapterInit, I219vEvtAdapterStart);
    NetAdapterInitSetAdapterStopCallback(AdapterInit, I219vEvtAdapterStop);
    // Обработчики Pause/Restart (уже были в Adapter.c и зарегистрированы)
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

    // Статические возможности (например, MAC-адрес, если он известен до создания NETADAPTER)
    // больше не устанавливаются здесь. Они будут установлены в I219vEvtAdapterSetCapabilities
    // или через специфичные для них обратные вызовы, если таковые используются (например, MAC-адрес).
    // I219vEvtAdapterSetLinkLayerAddress и I219vEvtAdapterSetPermanentLinkLayerAddress
    // по-прежнему могут быть зарегистрированы отдельно, если это необходимо для установки MAC-адреса
    // до того, как основной EvtAdapterSetCapabilities будет вызван, или для обновления.
    // Однако, если EvtAdapterSetCapabilities устанавливает все, то эти статические вызовы здесь могут быть избыточны.
    // Для данной консолидации, предполагается, что EvtAdapterSetCapabilities управляет всем.
    // Но MAC адрес часто устанавливается через NetAdapterInitSetCurrentLinkLayerAddress / NetAdapterInitSetPermanentLinkLayerAddress
    // Так как DeviceContext->MacAddress известен на этом этапе.

    // Установка MAC-адреса (пример сохранения, если это необходимо до EvtAdapterSetCapabilities)
    // Эти функции также могут быть частью EvtAdapterSetCapabilities если логика этого требует.
    // Для ясности, оставим их здесь, так как они специфичны и не являются "общими" возможностями.
    NetAdapterInitSetLinkLayerAddressCallback(AdapterInit, I219vEvtAdapterSetLinkLayerAddress, DeviceContext);
    NetAdapterInitSetPermanentLinkLayerAddressCallback(AdapterInit, I219vEvtAdapterSetPermanentLinkLayerAddress, DeviceContext);
    // Также CurrentLinkState часто является отдельным коллбэком.
    NetAdapterInitSetCurrentLinkStateCallback(AdapterInit, I219vEvtAdapterSetCurrentLinkState, DeviceContext);


    // Большинство NetAdapterSetXxxCapabilities (статических) удалено отсюда,
    // так как они теперь должны быть установлены в рамках коллбэка I219vEvtAdapterSetCapabilities.

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Adapter callbacks registered successfully (consolidated)");
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
