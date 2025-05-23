#pragma once

/*++

Copyright (c) 2025 Manus AI

Module Name:

    Queue.h

Abstract:

    Заголовочный файл для модуля очередей.
    Содержит объявления функций и структур, используемых в Queue.c

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <netadaptercx.h>

// Константы для размеров колец дескрипторов
#define I219V_RX_RING_SIZE 256
#define I219V_TX_RING_SIZE 256

// Объявление обработчиков очередей
EVT_PACKET_QUEUE_ADVANCE I219vEvtRxQueueAdvance;
EVT_PACKET_QUEUE_ADVANCE I219vEvtTxQueueAdvance;

// Объявление обработчиков прерываний
EVT_WDF_INTERRUPT_ISR I219vEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC I219vEvtInterruptDpc;

// Объявление вспомогательных функций
NTSTATUS I219vInitializeInterrupt(_In_ WDFDEVICE Device);
NTSTATUS I219vInitializeQueues(_In_ struct _I219V_DEVICE_CONTEXT* DeviceContext);
