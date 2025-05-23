/*++

Copyright (c) 2025 Manus AI

Module Name:

    Datapath.c

Abstract:

    Реализация функций для работы с путями данных (datapath) в NetAdapterCx
    Содержит функции для инициализации и управления кольцами дескрипторов

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
#include "Trace.h"

// Структура дескриптора приема
typedef struct _I219V_RX_DESC {
    UINT64 BufferAddr;    // Адрес буфера
    UINT16 Length;        // Длина принятого пакета
    UINT16 Checksum;      // Контрольная сумма
    UINT8  Status;        // Статус дескриптора
    UINT8  Errors;        // Ошибки
    UINT16 VlanTag;       // VLAN тег
} I219V_RX_DESC, *PI219V_RX_DESC;

// Структура дескриптора передачи
typedef struct _I219V_TX_DESC {
    UINT64 BufferAddr;    // Адрес буфера
    UINT16 Length;        // Длина пакета
    UINT8  CSO;           // Смещение контрольной суммы
    UINT8  CMD;           // Команды
    UINT8  Status;        // Статус
    UINT8  CSS;           // Смещение начала контрольной суммы
    UINT16 Special;       // Специальные поля
} I219V_TX_DESC, *PI219V_TX_DESC;

// Инициализация кольца дескрипторов приема
NTSTATUS
I219vInitializeRxRing(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_COMMON_BUFFER_CONFIG commonBufferConfig;
    WDFCOMMONBUFFER rxRingBuffer = NULL;
    PHYSICAL_ADDRESS rxRingPA;
    PI219V_RX_DESC rxRing;
    SIZE_T rxRingSize;
    UINT32 i;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Initializing RX ring");

    // Настройка конфигурации общего буфера
    WDF_COMMON_BUFFER_CONFIG_INIT(&commonBufferConfig, 0);

    // Создание общего буфера для кольца дескрипторов приема
    rxRingSize = I219V_RX_RING_SIZE * sizeof(I219V_RX_DESC);
    status = WdfCommonBufferCreate(
        WdfDeviceGetDmaEnabler(DeviceContext->Device),
        rxRingSize,
        &commonBufferConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &rxRingBuffer
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DATAPATH, 
                  "WdfCommonBufferCreate for RX ring failed %!STATUS!", status);
        return status;
    }

    // Получение виртуального адреса кольца дескрипторов
    rxRing = (PI219V_RX_DESC)WdfCommonBufferGetAlignedVirtualAddress(rxRingBuffer);
    RtlZeroMemory(rxRing, rxRingSize);

    // Получение физического адреса кольца дескрипторов
    rxRingPA = WdfCommonBufferGetAlignedLogicalAddress(rxRingBuffer);

    // Инициализация дескрипторов приема
    for (i = 0; i < I219V_RX_RING_SIZE; i++) {
        // В реальной реализации здесь бы выделялись буферы для приема данных
        // и их адреса записывались бы в дескрипторы
        rxRing[i].BufferAddr = 0;
        rxRing[i].Status = 0;
    }

    // Сохранение информации о кольце дескрипторов в контексте устройства
    DeviceContext->RxRingBuffer = rxRingBuffer;
    DeviceContext->RxRing = rxRing;
    DeviceContext->RxRingPA = rxRingPA;

    // Настройка регистров устройства
    I219vWriteRegister(DeviceContext, I219V_REG_RDBAL, (UINT32)rxRingPA.LowPart);
    I219vWriteRegister(DeviceContext, I219V_REG_RDBAH, (UINT32)rxRingPA.HighPart);
    I219vWriteRegister(DeviceContext, I219V_REG_RDLEN, (UINT32)rxRingSize);
    I219vWriteRegister(DeviceContext, I219V_REG_RDH, 0);
    I219vWriteRegister(DeviceContext, I219V_REG_RDT, I219V_RX_RING_SIZE - 1);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, 
              "RX ring initialized: VA=%p, PA=0x%llx, Size=%llu", 
              rxRing, rxRingPA.QuadPart, (ULONGLONG)rxRingSize);

    return status;
}

// Инициализация кольца дескрипторов передачи
NTSTATUS
I219vInitializeTxRing(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_COMMON_BUFFER_CONFIG commonBufferConfig;
    WDFCOMMONBUFFER txRingBuffer = NULL;
    PHYSICAL_ADDRESS txRingPA;
    PI219V_TX_DESC txRing;
    SIZE_T txRingSize;
    UINT32 i;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Initializing TX ring");

    // Настройка конфигурации общего буфера
    WDF_COMMON_BUFFER_CONFIG_INIT(&commonBufferConfig, 0);

    // Создание общего буфера для кольца дескрипторов передачи
    txRingSize = I219V_TX_RING_SIZE * sizeof(I219V_TX_DESC);
    status = WdfCommonBufferCreate(
        WdfDeviceGetDmaEnabler(DeviceContext->Device),
        txRingSize,
        &commonBufferConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &txRingBuffer
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DATAPATH, 
                  "WdfCommonBufferCreate for TX ring failed %!STATUS!", status);
        return status;
    }

    // Получение виртуального адреса кольца дескрипторов
    txRing = (PI219V_TX_DESC)WdfCommonBufferGetAlignedVirtualAddress(txRingBuffer);
    RtlZeroMemory(txRing, txRingSize);

    // Получение физического адреса кольца дескрипторов
    txRingPA = WdfCommonBufferGetAlignedLogicalAddress(txRingBuffer);

    // Инициализация дескрипторов передачи
    for (i = 0; i < I219V_TX_RING_SIZE; i++) {
        txRing[i].BufferAddr = 0;
        txRing[i].Status = 0;
        txRing[i].CMD = 0;
    }

    // Сохранение информации о кольце дескрипторов в контексте устройства
    DeviceContext->TxRingBuffer = txRingBuffer;
    DeviceContext->TxRing = txRing;
    DeviceContext->TxRingPA = txRingPA;

    // Настройка регистров устройства
    I219vWriteRegister(DeviceContext, I219V_REG_TDBAL, (UINT32)txRingPA.LowPart);
    I219vWriteRegister(DeviceContext, I219V_REG_TDBAH, (UINT32)txRingPA.HighPart);
    I219vWriteRegister(DeviceContext, I219V_REG_TDLEN, (UINT32)txRingSize);
    I219vWriteRegister(DeviceContext, I219V_REG_TDH, 0);
    I219vWriteRegister(DeviceContext, I219V_REG_TDT, 0);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, 
              "TX ring initialized: VA=%p, PA=0x%llx, Size=%llu", 
              txRing, txRingPA.QuadPart, (ULONGLONG)txRingSize);

    return status;
}

// Освобождение ресурсов колец дескрипторов
VOID
I219vCleanupRings(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Cleaning up rings");

    // Освобождение кольца дескрипторов приема
    if (DeviceContext->RxRingBuffer != NULL) {
        WdfObjectDelete(DeviceContext->RxRingBuffer);
        DeviceContext->RxRingBuffer = NULL;
        DeviceContext->RxRing = NULL;
    }

    // Освобождение кольца дескрипторов передачи
    if (DeviceContext->TxRingBuffer != NULL) {
        WdfObjectDelete(DeviceContext->TxRingBuffer);
        DeviceContext->TxRingBuffer = NULL;
        DeviceContext->TxRing = NULL;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Rings cleaned up");
}

// Инициализация DMA для устройства
NTSTATUS
I219vInitializeDma(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status;
    WDF_DMA_ENABLER_CONFIG dmaConfig;
    WDFDMAENABLER dmaEnabler;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Initializing DMA");

    // Настройка конфигурации DMA
    WDF_DMA_ENABLER_CONFIG_INIT(&dmaConfig, WdfDmaProfileScatterGather64, I219V_MAX_PACKET_SIZE);
    dmaConfig.WdmDmaVersionOverride = 3;

    // Создание DMA Enabler
    status = WdfDmaEnablerCreate(
        DeviceContext->Device,
        &dmaConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &dmaEnabler
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DATAPATH, 
                  "WdfDmaEnablerCreate failed %!STATUS!", status);
        return status;
    }

    // Сохранение DMA Enabler в контексте устройства
    DeviceContext->DmaEnabler = dmaEnabler;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "DMA initialized");

    return status;
}

// Инициализация путей данных
NTSTATUS
I219vInitializeDatapath(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Initializing datapath");

    // Инициализация DMA
    status = I219vInitializeDma(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DATAPATH, 
                  "I219vInitializeDma failed %!STATUS!", status);
        return status;
    }

    // Инициализация кольца дескрипторов приема
    status = I219vInitializeRxRing(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DATAPATH, 
                  "I219vInitializeRxRing failed %!STATUS!", status);
        goto Cleanup;
    }

    // Инициализация кольца дескрипторов передачи
    status = I219vInitializeTxRing(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DATAPATH, 
                  "I219vInitializeTxRing failed %!STATUS!", status);
        goto Cleanup;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Datapath initialized successfully");
    return STATUS_SUCCESS;

Cleanup:
    // В случае ошибки освобождаем ресурсы
    I219vCleanupRings(DeviceContext);
    return status;
}

// Очистка ресурсов путей данных
VOID
I219vCleanupDatapath(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Cleaning up datapath");

    // Освобождение колец дескрипторов
    I219vCleanupRings(DeviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DATAPATH, "Datapath cleaned up");
}
