#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_gaming.h

Abstract:

    Заголовочный файл для модуля игровых оптимизаций Intel i219-v.
    Содержит объявления функций и структур для оптимизации сетевой производительности в играх.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Типы игровых профилей
typedef enum _I219V_GAMING_PROFILE_TYPE {
    I219V_GAMING_PROFILE_BALANCED = 0,      // Сбалансированный профиль
    I219V_GAMING_PROFILE_COMPETITIVE = 1,   // Профиль для соревновательных игр (минимальная задержка)
    I219V_GAMING_PROFILE_STREAMING = 2,     // Профиль для стриминга игр
    I219V_GAMING_PROFILE_CUSTOM = 3         // Пользовательский профиль
} I219V_GAMING_PROFILE_TYPE;

// Уровни приоритизации трафика
typedef enum _I219V_TRAFFIC_PRIORITY_LEVEL {
    I219V_TRAFFIC_PRIORITY_HIGHEST = 0,     // Наивысший приоритет (игровой трафик)
    I219V_TRAFFIC_PRIORITY_HIGH = 1,        // Высокий приоритет (голосовой чат)
    I219V_TRAFFIC_PRIORITY_MEDIUM = 2,      // Средний приоритет (стриминг)
    I219V_TRAFFIC_PRIORITY_LOW = 3,         // Низкий приоритет (загрузки)
    I219V_TRAFFIC_PRIORITY_LOWEST = 4       // Наименьший приоритет (фоновые задачи)
} I219V_TRAFFIC_PRIORITY_LEVEL;

// Структура игрового профиля
typedef struct _I219V_GAMING_PROFILE {
    I219V_GAMING_PROFILE_TYPE ProfileType;          // Тип профиля
    BOOLEAN EnableTrafficPrioritization;            // Включение приоритизации трафика
    BOOLEAN EnableLatencyReduction;                 // Включение снижения задержки
    BOOLEAN EnableBandwidthControl;                 // Включение контроля пропускной способности
    BOOLEAN EnableSmartPowerManagement;             // Включение интеллектуального управления энергопотреблением
    UINT32 ReceiveBufferSize;                       // Размер буфера приема
    UINT32 TransmitBufferSize;                      // Размер буфера передачи
    UINT32 InterruptModeration;                     // Уровень модерации прерываний (0-100)
    UINT32 ReceiveDescriptors;                      // Количество дескрипторов приема
    UINT32 TransmitDescriptors;                     // Количество дескрипторов передачи
} I219V_GAMING_PROFILE, *PI219V_GAMING_PROFILE;

// Структура для отслеживания статистики производительности
typedef struct _I219V_GAMING_PERFORMANCE_STATS {
    UINT64 TotalPacketsSent;                        // Общее количество отправленных пакетов
    UINT64 TotalPacketsReceived;                    // Общее количество полученных пакетов
    UINT64 HighPriorityPacketsSent;                 // Количество отправленных пакетов с высоким приоритетом
    UINT64 HighPriorityPacketsReceived;             // Количество полученных пакетов с высоким приоритетом
    UINT64 LowLatencyPacketsSent;                   // Количество отправленных пакетов с низкой задержкой
    UINT64 LowLatencyPacketsReceived;               // Количество полученных пакетов с низкой задержкой
    UINT32 CurrentLatencyMs;                        // Текущая задержка в миллисекундах
    UINT32 AverageLatencyMs;                        // Средняя задержка в миллисекундах
    UINT32 PeakLatencyMs;                           // Пиковая задержка в миллисекундах
    UINT32 CurrentBandwidthKbps;                    // Текущая пропускная способность в Кбит/с
    UINT32 AverageBandwidthKbps;                    // Средняя пропускная способность в Кбит/с
    UINT32 PeakBandwidthKbps;                       // Пиковая пропускная способность в Кбит/с
} I219V_GAMING_PERFORMANCE_STATS, *PI219V_GAMING_PERFORMANCE_STATS;

// Объявление функций для игровых оптимизаций
NTSTATUS I219vInitializeGamingFeatures(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vApplyGamingProfile(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ PI219V_GAMING_PROFILE GamingProfile);
NTSTATUS I219vEnableTrafficPrioritization(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN Enable);
NTSTATUS I219vEnableLatencyReduction(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN Enable);
NTSTATUS I219vEnableBandwidthControl(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN Enable);
NTSTATUS I219vEnableSmartPowerManagement(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN Enable);
NTSTATUS I219vSetPacketPriority(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ PNET_PACKET Packet, _In_ I219V_TRAFFIC_PRIORITY_LEVEL Priority);
NTSTATUS I219vOptimizeBuffersForGaming(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vOptimizeInterruptsForGaming(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vGetGamingPerformanceStats(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _Out_ PI219V_GAMING_PERFORMANCE_STATS PerformanceStats);

// Функции для получения предопределенных игровых профилей
VOID I219vGetDefaultGamingProfile(_Out_ PI219V_GAMING_PROFILE GamingProfile);
VOID I219vGetCompetitiveGamingProfile(_Out_ PI219V_GAMING_PROFILE GamingProfile);
VOID I219vGetStreamingGamingProfile(_Out_ PI219V_GAMING_PROFILE GamingProfile);

// Функции для анализа и классификации трафика
BOOLEAN I219vIsGamingTraffic(_In_ PNET_PACKET Packet);
BOOLEAN I219vIsVoiceTraffic(_In_ PNET_PACKET Packet);
BOOLEAN I219vIsStreamingTraffic(_In_ PNET_PACKET Packet);
BOOLEAN I219vIsBackgroundTraffic(_In_ PNET_PACKET Packet);

// Функции для взаимодействия с пользовательским режимом
NTSTATUS I219vRegisterGamingInterface(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vHandleGamingIoctl(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ WDFREQUEST Request);
