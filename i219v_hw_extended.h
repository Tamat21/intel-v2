#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_hw_extended.h

Abstract:

    Расширенные определения регистров и констант для Intel i219-v.
    Содержит дополнительные определения для поддержки игровых функций.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Дополнительные регистры для игровых функций
#define I219V_REG_TQAVCC        0x3004  // Регистр управления приоритетами трафика
#define I219V_REG_ITR           0x00C4  // Регистр модерации прерываний

// Биты для регистра TXCW
#define I219V_TXCW_QOS_ENABLE   0x00000400  // Бит включения QoS

// Биты для регистра RXCW
#define I219V_RXCW_QOS_ENABLE   0x00000400  // Бит включения QoS

// Биты для регистра CTRL
#define I219V_CTRL_ITR_ENABLE   0x00004000  // Бит включения модерации прерываний
#define I219V_CTRL_EEE_ENABLE   0x00100000  // Бит включения Energy Efficient Ethernet
#define I219V_CTRL_ASPM_ENABLE  0x00200000  // Бит включения автоматического управления энергопотреблением

// Биты для регистра RXDCTL
#define I219V_RXDCTL_PTHRESH_MASK   0x0000003F  // Маска порога дескрипторов приема
#define I219V_RXDCTL_PTHRESH_SHIFT  0           // Сдвиг порога дескрипторов приема

// Биты для регистра TXDCTL
#define I219V_TXDCTL_PTHRESH_MASK   0x0000003F  // Маска порога дескрипторов передачи
#define I219V_TXDCTL_PTHRESH_SHIFT  0           // Сдвиг порога дескрипторов передачи

// Значения для регистра TQAVCC
#define I219V_TQAVCC_QOS_ENABLE         0x00000001  // Бит включения QoS
#define I219V_TQAVCC_GAMING_PRIORITY    0x00000101  // Значение для игрового приоритета
