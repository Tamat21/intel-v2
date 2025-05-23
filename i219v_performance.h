#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_performance.h

Abstract:

    Заголовочный файл для модуля оптимизации производительности Intel i219-v.
    Содержит объявления функций и структур для оптимизации производительности.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Размеры колец дескрипторов
#define I219V_RX_RING_SIZE    256
#define I219V_TX_RING_SIZE    256

// Типы профилей производительности
typedef enum _I219V_PERFORMANCE_PROFILE_TYPE {
    I219V_PERFORMANCE_PROFILE_BALANCED = 0,     // Сбалансированный профиль
    I219V_PERFORMANCE_PROFILE_THROUGHPUT = 1,   // Профиль для максимальной пропускной способности
    I219V_PERFORMANCE_PROFILE_LATENCY = 2,      // Профиль для минимальной задержки
    I219V_PERFORMANCE_PROFILE_POWER_SAVING = 3  // Профиль для энергосбережения
} I219V_PERFORMANCE_PROFILE_TYPE;

// Уровни модерации прерываний
typedef enum _I219V_INTERRUPT_MODERATION_LEVEL {
    I219V_INTERRUPT_MODERATION_DISABLED = 0,  // Модерация отключена
    I219V_INTERRUPT_MODERATION_LOW = 1,       // Низкий уровень модерации (минимальная задержка)
    I219V_INTERRUPT_MODERATION_MEDIUM = 2,    // Средний уровень модерации (баланс)
    I219V_INTERRUPT_MODERATION_HIGH = 3       // Высокий уровень модерации (максимальная пропускная способность)
} I219V_INTERRUPT_MODERATION_LEVEL;

// Структура профиля производительности
typedef struct _I219V_PERFORMANCE_PROFILE {
    I219V_PERFORMANCE_PROFILE_TYPE ProfileType;       // Тип профиля
    I219V_INTERRUPT_MODERATION_LEVEL InterruptModerationLevel;  // Уровень модерации прерываний
    BOOLEAN EnableEnergyEfficiency;                   // Включение энергосбережения
    UINT32 RxRingSize;                                // Размер кольца приема
    UINT32 TxRingSize;                                // Размер кольца передачи
    UINT32 RxBufferSize;                              // Размер буфера приема
    UINT32 MaxRxQueues;                               // Максимальное количество очередей приема
    UINT32 MaxTxQueues;                               // Максимальное количество очередей передачи
} I219V_PERFORMANCE_PROFILE, *PI219V_PERFORMANCE_PROFILE;

// Объявление функций для оптимизации производительности
NTSTATUS I219vOptimizeInterrupts(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ I219V_INTERRUPT_MODERATION_LEVEL ModerationLevel);
NTSTATUS I219vOptimizeDma(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vOptimizeBufferSizes(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vOptimizePowerManagement(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN EnableEnergyEfficiency);
NTSTATUS I219vOptimizeTransmitParameters(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vOptimizeReceiveParameters(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vApplyPerformanceOptimizations(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ PI219V_PERFORMANCE_PROFILE PerformanceProfile);

// Функции для получения предопределенных профилей производительности
VOID I219vGetDefaultPerformanceProfile(_Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile);
VOID I219vGetThroughputPerformanceProfile(_Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile);
VOID I219vGetLatencyPerformanceProfile(_Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile);
VOID I219vGetPowerSavingPerformanceProfile(_Out_ PI219V_PERFORMANCE_PROFILE PerformanceProfile);
