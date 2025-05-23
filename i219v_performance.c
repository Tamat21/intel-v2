/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_performance.c

Abstract:

    Реализация функций для оптимизации производительности драйвера Intel i219-v
    Содержит функции настройки прерываний, DMA и других параметров для повышения производительности

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
#include "i219v_performance.h"
#include "DeviceContext.h"
#include "Trace.h"

// Оптимизация параметров прерываний
NTSTATUS
I219vOptimizeInterrupts(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _In_ I219V_INTERRUPT_MODERATION_LEVEL ModerationLevel
    )
{
    UINT32 eitr0;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Optimizing interrupts, moderation level: %d", ModerationLevel);

    // Настройка регистра EITR0 (Extended Interrupt Throttle Register)
    // Этот регистр контролирует коалесцинг прерываний
    switch (ModerationLevel) {
    case I219V_INTERRUPT_MODERATION_DISABLED:
        // Отключение модерации прерываний
        eitr0 = 0;
        break;
        
    case I219V_INTERRUPT_MODERATION_LOW:
        // Низкий уровень модерации (минимальная задержка)
        // ITR = 2 мкс (значение 2)
        eitr0 = 2;
        break;
        
    case I219V_INTERRUPT_MODERATION_MEDIUM:
        // Средний уровень модерации (баланс между задержкой и пропускной способностью)
        // ITR = 20 мкс (значение 20)
        eitr0 = 20;
        break;
        
    case I219V_INTERRUPT_MODERATION_HIGH:
        // Высокий уровень модерации (максимальная пропускная способность)
        // ITR = 200 мкс (значение 200)
        eitr0 = 200;
        break;
        
    default:
        // По умолчанию - средний уровень модерации
        eitr0 = 20;
        break;
    }

    // Запись регистра EITR0
    I219vWriteRegister(DeviceContext, I219V_REG_EITR0, eitr0);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Interrupt moderation configured, EITR0: 0x%08x", eitr0);

    // Настройка регистра IAM (Interrupt Acknowledge Auto-Mask)
    // Этот регистр контролирует автоматическую маскировку прерываний
    if (ModerationLevel != I219V_INTERRUPT_MODERATION_DISABLED) {
        // Включение автоматической маскировки прерываний
        I219vWriteRegister(DeviceContext, I219V_REG_EIAM, 0xFFFFFFFF);
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
                  "Interrupt auto-mask enabled");
    } else {
        // Отключение автоматической маскировки прерываний
        I219vWriteRegister(DeviceContext, I219V_REG_EIAM, 0);
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
                  "Interrupt auto-mask disabled");
    }

    return STATUS_SUCCESS;
}

// Оптимизация параметров DMA
NTSTATUS
I219vOptimizeDma(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    UINT32 rxdctl, txdctl;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Optimizing DMA parameters");

    // Настройка параметров DMA для приема
    rxdctl = I219vReadRegister(DeviceContext, I219V_REG_RXDCTL);
    
    // Установка порогов предвыборки, хоста и записи
    rxdctl &= ~(I219V_RXDCTL_PTHRESH_MASK | I219V_RXDCTL_HTHRESH_MASK | I219V_RXDCTL_WTHRESH_MASK);
    rxdctl |= (8 & 0x3F) |                // PTHRESH = 8
              ((4 & 0x3F) << 8) |         // HTHRESH = 4
              ((4 & 0x3F) << 16);         // WTHRESH = 4
    
    // Установка гранулярности в дескрипторах
    rxdctl &= ~I219V_RXDCTL_GRAN;
    
    // Запись регистра RXDCTL
    I219vWriteRegister(DeviceContext, I219V_REG_RXDCTL, rxdctl);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "DMA receive parameters configured, RXDCTL: 0x%08x", rxdctl);

    // Настройка параметров DMA для передачи
    txdctl = I219vReadRegister(DeviceContext, I219V_REG_TXDCTL);
    
    // Установка порогов предвыборки, хоста и записи
    txdctl &= ~(I219V_TXDCTL_PTHRESH_MASK | I219V_TXDCTL_HTHRESH_MASK | I219V_TXDCTL_WTHRESH_MASK);
    txdctl |= (8 & 0x3F) |                // PTHRESH = 8
              ((4 & 0x3F) << 8) |         // HTHRESH = 4
              ((4 & 0x3F) << 16);         // WTHRESH = 4
    
    // Установка гранулярности в дескрипторах
    txdctl &= ~I219V_TXDCTL_GRAN;
    
    // Запись регистра TXDCTL
    I219vWriteRegister(DeviceContext, I219V_REG_TXDCTL, txdctl);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "DMA transmit parameters configured, TXDCTL: 0x%08x", txdctl);

    return STATUS_SUCCESS;
}

// Оптимизация размеров буферов
NTSTATUS
I219vOptimizeBufferSizes(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    UINT32 rctl;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Optimizing buffer sizes");

    // Настройка размеров буферов приема
    rctl = I219vReadRegister(DeviceContext, I219V_REG_RCTL);
    
    // Очистка битов размера буфера
    rctl &= ~I219V_RCTL_FLXBUF_MASK;
    
    // Установка размера буфера 2K
    rctl |= I219V_RCTL_FLXBUF_2K;
    
    // Запись регистра RCTL
    I219vWriteRegister(DeviceContext, I219V_REG_RCTL, rctl);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Buffer sizes configured, RCTL: 0x%08x", rctl);

    return STATUS_SUCCESS;
}

// Оптимизация параметров энергосбережения
NTSTATUS
I219vOptimizePowerManagement(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _In_ BOOLEAN EnableEnergyEfficiency
    )
{
    UINT32 eeer;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Optimizing power management, energy efficiency: %s", 
              EnableEnergyEfficiency ? "enabled" : "disabled");

    // Настройка Energy Efficient Ethernet (EEE)
    eeer = I219vReadRegister(DeviceContext, I219V_REG_EEER);
    
    if (EnableEnergyEfficiency) {
        // Включение EEE
        eeer |= I219V_EEER_TX_LPI_EN | I219V_EEER_RX_LPI_EN;
    } else {
        // Отключение EEE
        eeer &= ~(I219V_EEER_TX_LPI_EN | I219V_EEER_RX_LPI_EN);
    }
    
    // Запись регистра EEER
    I219vWriteRegister(DeviceContext, I219V_REG_EEER, eeer);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Power management configured, EEER: 0x%08x", eeer);

    return STATUS_SUCCESS;
}

// Оптимизация параметров передачи
NTSTATUS
I219vOptimizeTransmitParameters(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    UINT32 tipg, tctl;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Optimizing transmit parameters");

    // Настройка Inter-Packet Gap (IPG)
    tipg = 0;
    tipg |= 8;      // IPGT (IPG Transmit Time) = 8
    tipg |= 8 << 10;  // IPGR1 (IPG Receive Time 1) = 8
    tipg |= 6 << 20;  // IPGR2 (IPG Receive Time 2) = 6
    
    // Запись регистра TIPG
    I219vWriteRegister(DeviceContext, I219V_REG_TIPG, tipg);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Transmit IPG configured, TIPG: 0x%08x", tipg);

    // Настройка параметров передачи
    tctl = I219vReadRegister(DeviceContext, I219V_REG_TCTL);
    
    // Установка порога коллизий (CT)
    tctl &= ~I219V_TCTL_CT_MASK;
    tctl |= I219V_TCTL_CT_DEF;
    
    // Установка расстояния коллизий (COLD)
    tctl &= ~I219V_TCTL_COLD_MASK;
    tctl |= I219V_TCTL_COLD_DEF;
    
    // Запись регистра TCTL
    I219vWriteRegister(DeviceContext, I219V_REG_TCTL, tctl);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Transmit parameters configured, TCTL: 0x%08x", tctl);

    return STATUS_SUCCESS;
}

// Оптимизация параметров приема
NTSTATUS
I219vOptimizeReceiveParameters(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    UINT32 rctl, rfctl;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Optimizing receive parameters");

    // Настройка параметров приема
    rctl = I219vReadRegister(DeviceContext, I219V_REG_RCTL);
    
    // Установка типа дескриптора
    rctl &= ~I219V_RCTL_DTYP_MASK;
    rctl |= I219V_RCTL_DTYP_ADV;  // Использование расширенных дескрипторов
    
    // Запись регистра RCTL
    I219vWriteRegister(DeviceContext, I219V_REG_RCTL, rctl);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Receive parameters configured, RCTL: 0x%08x", rctl);

    // Настройка фильтрации приема
    rfctl = I219vReadRegister(DeviceContext, I219V_REG_RFCTL);
    
    // Запись регистра RFCTL
    I219vWriteRegister(DeviceContext, I219V_REG_RFCTL, rfctl);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Receive filter configured, RFCTL: 0x%08x", rfctl);

    return STATUS_SUCCESS;
}

// Применение оптимизаций производительности
NTSTATUS
I219vApplyPerformanceOptimizations(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _In_ PI219V_PERFORMANCE_PROFILE PerformanceProfile
    )
{
    NTSTATUS status;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Applying performance optimizations, profile: %d", 
              PerformanceProfile->ProfileType);

    // Оптимизация прерываний
    status = I219vOptimizeInterrupts(DeviceContext, PerformanceProfile->InterruptModerationLevel);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vOptimizeInterrupts failed %!STATUS!", status);
        return status;
    }

    // Оптимизация DMA
    status = I219vOptimizeDma(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vOptimizeDma failed %!STATUS!", status);
        return status;
    }

    // Оптимизация размеров буферов
    status = I219vOptimizeBufferSizes(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vOptimizeBufferSizes failed %!STATUS!", status);
        return status;
    }

    // Оптимизация параметров энергосбережения
    status = I219vOptimizePowerManagement(DeviceContext, PerformanceProfile->EnableEnergyEfficiency);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vOptimizePowerManagement failed %!STATUS!", status);
        return status;
    }

    // Оптимизация параметров передачи
    status = I219vOptimizeTransmitParameters(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vOptimizeTransmitParameters failed %!STATUS!", status);
        return status;
    }

    // Оптимизация параметров приема
    status = I219vOptimizeReceiveParameters(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vOptimizeReceiveParameters failed %!STATUS!", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Performance optimizations applied successfully");

    return STATUS_SUCCESS;
}

// Получение профиля производительности по умолчанию
VOID
I219vGetDefaultPerformanceProfile(
    _Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile
    )
{
    // Очистка структуры профиля
    RtlZeroMemory(PerformanceProfile, sizeof(I219V_PERFORMANCE_PROFILE));

    // Установка значений по умолчанию
    PerformanceProfile->ProfileType = I219V_PERFORMANCE_PROFILE_BALANCED;
    PerformanceProfile->InterruptModerationLevel = I219V_INTERRUPT_MODERATION_MEDIUM;
    PerformanceProfile->EnableEnergyEfficiency = TRUE;
    PerformanceProfile->RxRingSize = I219V_RX_RING_SIZE;
    PerformanceProfile->TxRingSize = I219V_TX_RING_SIZE;
    PerformanceProfile->RxBufferSize = 2048;  // 2K
    PerformanceProfile->MaxRxQueues = 1;
    PerformanceProfile->MaxTxQueues = 1;
}

// Получение профиля производительности для максимальной пропускной способности
VOID
I219vGetThroughputPerformanceProfile(
    _Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile
    )
{
    // Очистка структуры профиля
    RtlZeroMemory(PerformanceProfile, sizeof(I219V_PERFORMANCE_PROFILE));

    // Установка значений для максимальной пропускной способности
    PerformanceProfile->ProfileType = I219V_PERFORMANCE_PROFILE_THROUGHPUT;
    PerformanceProfile->InterruptModerationLevel = I219V_INTERRUPT_MODERATION_HIGH;
    PerformanceProfile->EnableEnergyEfficiency = FALSE;
    PerformanceProfile->RxRingSize = I219V_RX_RING_SIZE;
    PerformanceProfile->TxRingSize = I219V_TX_RING_SIZE;
    PerformanceProfile->RxBufferSize = 4096;  // 4K
    PerformanceProfile->MaxRxQueues = 1;
    PerformanceProfile->MaxTxQueues = 1;
}

// Получение профиля производительности для минимальной задержки
VOID
I219vGetLatencyPerformanceProfile(
    _Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile
    )
{
    // Очистка структуры профиля
    RtlZeroMemory(PerformanceProfile, sizeof(I219V_PERFORMANCE_PROFILE));

    // Установка значений для минимальной задержки
    PerformanceProfile->ProfileType = I219V_PERFORMANCE_PROFILE_LATENCY;
    PerformanceProfile->InterruptModerationLevel = I219V_INTERRUPT_MODERATION_LOW;
    PerformanceProfile->EnableEnergyEfficiency = FALSE;
    PerformanceProfile->RxRingSize = I219V_RX_RING_SIZE;
    PerformanceProfile->TxRingSize = I219V_TX_RING_SIZE;
    PerformanceProfile->RxBufferSize = 2048;  // 2K
    PerformanceProfile->MaxRxQueues = 1;
    PerformanceProfile->MaxTxQueues = 1;
}

// Получение профиля производительности для энергосбережения
VOID
I219vGetPowerSavingPerformanceProfile(
    _Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile
    )
{
    // Очистка структуры профиля
    RtlZeroMemory(PerformanceProfile, sizeof(I219V_PERFORMANCE_PROFILE));

    // Установка значений для энергосбережения
    PerformanceProfile->ProfileType = I219V_PERFORMANCE_PROFILE_POWER_SAVING;
    PerformanceProfile->InterruptModerationLevel = I219V_INTERRUPT_MODERATION_MEDIUM;
    PerformanceProfile->EnableEnergyEfficiency = TRUE;
    PerformanceProfile->RxRingSize = I219V_RX_RING_SIZE / 2;  // Уменьшенный размер кольца
    PerformanceProfile->TxRingSize = I219V_TX_RING_SIZE / 2;  // Уменьшенный размер кольца
    PerformanceProfile->RxBufferSize = 2048;  // 2K
    PerformanceProfile->MaxRxQueues = 1;
    PerformanceProfile->MaxTxQueues = 1;
}
