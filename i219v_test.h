#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_test.h

Abstract:

    Заголовочный файл для модуля тестирования и отладки Intel i219-v.
    Содержит объявления функций и структур для тестирования драйвера.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Структура результатов самодиагностики
typedef struct _I219V_SELF_TEST_RESULTS {
    BOOLEAN RegisterAccessTest;   // Тест доступа к регистрам
    BOOLEAN EepromTest;           // Тест EEPROM
    BOOLEAN MacAddressTest;       // Тест MAC-адреса
    BOOLEAN PhyAccessTest;        // Тест доступа к PHY
    BOOLEAN LinkStatusTest;       // Тест состояния соединения
    BOOLEAN InterruptTest;        // Тест прерываний
    BOOLEAN DmaTest;              // Тест DMA
    BOOLEAN TxTest;               // Тест передачи
    BOOLEAN RxTest;               // Тест приема
} I219V_SELF_TEST_RESULTS, *PI219V_SELF_TEST_RESULTS;

// Структура результатов тестирования
typedef struct _I219V_TEST_RESULTS {
    BOOLEAN RegisterTestPassed;       // Результат теста регистров
    BOOLEAN PhyTestPassed;            // Результат теста PHY
    BOOLEAN MacAddressTestPassed;     // Результат теста MAC-адреса
    BOOLEAN LinkStatusTestPassed;     // Результат теста состояния соединения
    BOOLEAN StatisticsTestPassed;     // Результат теста статистики
    BOOLEAN OffloadsTestPassed;       // Результат теста оффлоадов
    BOOLEAN SelfTestPassed;           // Результат самодиагностики
    I219V_SELF_TEST_RESULTS SelfTestResults;  // Детальные результаты самодиагностики
} I219V_TEST_RESULTS, *PI219V_TEST_RESULTS;

// Объявление функций для тестирования
NTSTATUS I219vRunSelfTest(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _Out_ PI219V_SELF_TEST_RESULTS TestResults);
NTSTATUS I219vTestRegisters(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vTestPhy(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vTestMacAddress(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vTestLinkStatus(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vTestStatistics(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vTestOffloads(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vRunAllTests(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _Out_ PI219V_TEST_RESULTS TestResults);
