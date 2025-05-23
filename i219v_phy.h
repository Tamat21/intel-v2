#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_phy.h

Abstract:

    Заголовочный файл для модуля PHY Intel i219-v.
    Содержит объявления функций и констант для работы с физическим уровнем.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Константы для PHY
#define I219V_PHY_TIMEOUT        100     // Таймаут операций PHY (в 10 мкс)
#define I219V_PHY_RESET_TIMEOUT  100     // Таймаут сброса PHY (в мс)

// Ожидаемые идентификаторы PHY для Intel i219-v
#define I219V_PHY_ID1_EXPECTED   0x0000  // Идентификатор 1
#define I219V_PHY_ID2_EXPECTED   0x0000  // Идентификатор 2
#define I219V_PHY_ID2_MASK       0xFFF0  // Маска для идентификатора 2

// Объявление функций для работы с PHY
USHORT I219vReadPhy(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ USHORT PhyRegister);
VOID I219vWritePhy(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ USHORT PhyRegister, _In_ USHORT PhyData);
NTSTATUS I219vResetPhy(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vInitializePhy(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vConfigureEee(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
VOID I219vGetLinkState(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _Out_ PNET_ADAPTER_LINK_STATE LinkState);
VOID I219vConfigurePowerManagement(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ BOOLEAN EnableWakeOnLan);
VOID I219vConfigureLeds(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
NTSTATUS I219vDiagnosticPhy(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
