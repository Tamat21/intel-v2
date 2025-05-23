#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    DeviceContext.h

Abstract:

    Определение структуры контекста устройства для драйвера Intel i219-v.
    Расширено для поддержки игровых функций и оптимизаций Killer Performance.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>
#include "i219v_gaming.h"

// Структура контекста устройства
typedef struct _I219V_DEVICE_CONTEXT {
    // Базовые поля
    WDFDEVICE Device;                      // Дескриптор устройства
    NETADAPTER NetAdapter;                 // Дескриптор сетевого адаптера
    BOOLEAN DeviceInitialized;             // Флаг инициализации устройства
    BOOLEAN AdapterInitialized;            // Флаг инициализации адаптера
    BOOLEAN LinkUp;                        // Флаг состояния соединения
    BOOLEAN NeedResetAdapter;              // Флаг необходимости перезапуска адаптера

    // Ресурсы устройства
    PHYSICAL_ADDRESS IoBasePA;             // Физический адрес базы ввода-вывода
    PVOID IoBase;                          // Виртуальный адрес базы ввода-вывода
    ULONG IoSize;                          // Размер области ввода-вывода
    ULONG InterruptVector;                 // Вектор прерывания
    ULONG InterruptLevel;                  // Уровень прерывания
    WDFINTERRUPT Interrupt;                // Дескриптор прерывания

    // Параметры адаптера
    UCHAR MacAddress[6];                   // MAC-адрес
    UINT32 LinkSpeed;                      // Скорость соединения
    BOOLEAN FullDuplex;                    // Флаг полного дуплекса
    UINT32 MTU;                            // Максимальный размер пакета

    // Параметры производительности
    UINT32 ReceiveBufferSize;              // Размер буфера приема
    UINT32 TransmitBufferSize;             // Размер буфера передачи
    UINT32 ReceiveDescriptors;             // Количество дескрипторов приема
    UINT32 TransmitDescriptors;            // Количество дескрипторов передачи
    UINT32 InterruptModeration;            // Уровень модерации прерываний

    // Игровые функции и оптимизации Killer Performance
    I219V_GAMING_PROFILE GamingProfile;                // Текущий игровой профиль
    I219V_GAMING_PERFORMANCE_STATS GamingPerformanceStats; // Статистика производительности
    BOOLEAN TrafficPrioritizationEnabled;  // Флаг включения приоритизации трафика
    BOOLEAN LatencyReductionEnabled;       // Флаг включения снижения задержки
    BOOLEAN BandwidthControlEnabled;       // Флаг включения контроля пропускной способности
    BOOLEAN SmartPowerManagementEnabled;   // Флаг включения интеллектуального управления энергопотреблением

    // Дополнительные поля для игровых оптимизаций
    UINT32 GameTrafficCount;               // Счетчик игрового трафика
    UINT32 VoiceTrafficCount;              // Счетчик голосового трафика
    UINT32 StreamingTrafficCount;          // Счетчик стримингового трафика
    UINT32 BackgroundTrafficCount;         // Счетчик фонового трафика
    UINT64 LastPerformanceUpdateTime;      // Время последнего обновления статистики производительности

} I219V_DEVICE_CONTEXT, *PI219V_DEVICE_CONTEXT;
