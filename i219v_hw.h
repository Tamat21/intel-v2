#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_hw.h

Abstract:

    Заголовочный файл для модуля аппаратного обеспечения Intel i219-v.
    Содержит определения регистров и констант для работы с устройством.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Определения регистров устройства Intel i219-v
#define I219V_REG_CTRL      0x0000  // Регистр управления
#define I219V_REG_STATUS    0x0008  // Регистр статуса
#define I219V_REG_EERD      0x0014  // EEPROM Read Register
#define I219V_REG_ICR       0x00C0  // Interrupt Cause Read Register
#define I219V_REG_IMS       0x00D0  // Interrupt Mask Set Register
#define I219V_REG_IMC       0x00D8  // Interrupt Mask Clear Register
#define I219V_REG_RCTL      0x0100  // Receive Control Register
#define I219V_REG_TCTL      0x0400  // Transmit Control Register
#define I219V_REG_RDBAL     0x2800  // Rx Descriptor Base Address Low
#define I219V_REG_RDBAH     0x2804  // Rx Descriptor Base Address High
#define I219V_REG_RDLEN     0x2808  // Rx Descriptor Length
#define I219V_REG_RDH       0x2810  // Rx Descriptor Head
#define I219V_REG_RDT       0x2818  // Rx Descriptor Tail
#define I219V_REG_TDBAL     0x3800  // Tx Descriptor Base Address Low
#define I219V_REG_TDBAH     0x3804  // Tx Descriptor Base Address High
#define I219V_REG_TDLEN     0x3808  // Tx Descriptor Length
#define I219V_REG_TDH       0x3810  // Tx Descriptor Head
#define I219V_REG_TDT       0x3818  // Tx Descriptor Tail
#define I219V_REG_RAL       0x5400  // Receive Address Low
#define I219V_REG_RAH       0x5404  // Receive Address High

// Биты регистра управления (CTRL)
#define I219V_CTRL_RST      0x04000000  // Device Reset
#define I219V_CTRL_SLU      0x00000040  // Set Link Up

// Биты регистра управления приемом (RCTL)
#define I219V_RCTL_EN       0x00000002  // Receiver Enable
#define I219V_RCTL_BAM      0x00008000  // Broadcast Accept Mode
#define I219V_RCTL_SECRC    0x04000000  // Strip Ethernet CRC

// Биты регистра управления передачей (TCTL)
#define I219V_TCTL_EN       0x00000002  // Transmit Enable
#define I219V_TCTL_PSP      0x00000008  // Pad Short Packets

// Биты регистра масок прерываний (IMS/IMC)
#define I219V_IMS_TXDW      0x00000001  // Transmit Descriptor Written Back
#define I219V_IMS_RXDW      0x00000080  // Receive Descriptor Written Back
#define I219V_IMS_LSC       0x00000004  // Link Status Change

// Объявление функций для работы с аппаратным обеспечением
UINT32 I219vReadRegister(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ UINT32 Register);
VOID I219vWriteRegister(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext, _In_ UINT32 Register, _In_ UINT32 Value);
