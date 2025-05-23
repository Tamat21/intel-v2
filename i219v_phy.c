/*++

Copyright (c) 2025 Manus AI

Module Name:

    i219v_phy.c

Abstract:

    Реализация функций для работы с PHY Intel i219-v
    Содержит функции инициализации и управления физическим уровнем

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
#include "i219v_phy.h"
#include "DeviceContext.h"
#include "Trace.h"

// Чтение регистра PHY
USHORT
I219vReadPhy(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _In_ USHORT PhyRegister
    )
{
    UINT32 mdic;
    UINT32 i;
    USHORT phyData = 0;

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_HARDWARE, 
              "Reading PHY register 0x%04x", PhyRegister);

    // Формирование команды чтения PHY
    mdic = ((UINT32)PhyRegister << 16) |
           (I219V_REG_PHYADDR << 21) |
           (1 << 27);  // Операция чтения

    // Запись команды в регистр MDIC
    I219vWriteRegister(DeviceContext, I219V_REG_PHYREG, mdic);

    // Ожидание завершения операции
    for (i = 0; i < I219V_PHY_TIMEOUT; i++) {
        // Задержка 10 мкс
        NdisStallExecution(10);

        // Чтение регистра MDIC
        mdic = I219vReadRegister(DeviceContext, I219V_REG_PHYREG);

        // Проверка бита готовности
        if (mdic & (1 << 28)) {
            // Операция завершена
            if (mdic & (1 << 30)) {
                // Ошибка доступа к PHY
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                          "PHY read error, register 0x%04x", PhyRegister);
                return 0;
            }

            // Извлечение данных
            phyData = (USHORT)(mdic & 0xFFFF);
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_HARDWARE, 
                      "PHY register 0x%04x = 0x%04x", PhyRegister, phyData);
            return phyData;
        }
    }

    // Таймаут операции
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
              "PHY read timeout, register 0x%04x", PhyRegister);
    return 0;
}

// Запись в регистр PHY
VOID
I219vWritePhy(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _In_ USHORT PhyRegister,
    _In_ USHORT PhyData
    )
{
    UINT32 mdic;
    UINT32 i;

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_HARDWARE, 
              "Writing PHY register 0x%04x = 0x%04x", PhyRegister, PhyData);

    // Формирование команды записи PHY
    mdic = ((UINT32)PhyRegister << 16) |
           (I219V_REG_PHYADDR << 21) |
           (2 << 27) |  // Операция записи
           PhyData;

    // Запись команды в регистр MDIC
    I219vWriteRegister(DeviceContext, I219V_REG_PHYREG, mdic);

    // Ожидание завершения операции
    for (i = 0; i < I219V_PHY_TIMEOUT; i++) {
        // Задержка 10 мкс
        NdisStallExecution(10);

        // Чтение регистра MDIC
        mdic = I219vReadRegister(DeviceContext, I219V_REG_PHYREG);

        // Проверка бита готовности
        if (mdic & (1 << 28)) {
            // Операция завершена
            if (mdic & (1 << 30)) {
                // Ошибка доступа к PHY
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                          "PHY write error, register 0x%04x", PhyRegister);
            }
            return;
        }
    }

    // Таймаут операции
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
              "PHY write timeout, register 0x%04x", PhyRegister);
}

// Сброс PHY
NTSTATUS
I219vResetPhy(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    USHORT phyCtrl;
    UINT32 i;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Resetting PHY");

    // Чтение регистра управления PHY
    phyCtrl = I219vReadPhy(DeviceContext, I219V_PHY_CONTROL);

    // Установка бита сброса
    phyCtrl |= I219V_PHY_CTRL_RESET;

    // Запись регистра управления PHY
    I219vWritePhy(DeviceContext, I219V_PHY_CONTROL, phyCtrl);

    // Ожидание завершения сброса
    for (i = 0; i < I219V_PHY_RESET_TIMEOUT; i++) {
        // Задержка 1 мс
        NdisMSleep(1000);

        // Чтение регистра управления PHY
        phyCtrl = I219vReadPhy(DeviceContext, I219V_PHY_CONTROL);

        // Проверка бита сброса
        if (!(phyCtrl & I219V_PHY_CTRL_RESET)) {
            // Сброс завершен
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "PHY reset completed");
            return STATUS_SUCCESS;
        }
    }

    // Таймаут сброса
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, "PHY reset timeout");
    return STATUS_DEVICE_NOT_READY;
}

// Инициализация PHY
NTSTATUS
I219vInitializePhy(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status;
    USHORT phyCtrl, phyStatus, phyId1, phyId2;
    USHORT phyAna, phy1000tCtrl;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Initializing PHY");

    // Сброс PHY
    status = I219vResetPhy(DeviceContext);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_HARDWARE, 
                  "I219vResetPhy failed %!STATUS!", status);
        return status;
    }

    // Чтение идентификаторов PHY
    phyId1 = I219vReadPhy(DeviceContext, I219V_PHY_ID1);
    phyId2 = I219vReadPhy(DeviceContext, I219V_PHY_ID2);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "PHY ID: 0x%04x 0x%04x", phyId1, phyId2);

    // Проверка идентификаторов PHY для i219-v
    if (phyId1 != I219V_PHY_ID1_EXPECTED || (phyId2 & I219V_PHY_ID2_MASK) != I219V_PHY_ID2_EXPECTED) {
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_HARDWARE, 
                  "Unexpected PHY ID: 0x%04x 0x%04x, expected: 0x%04x 0x%04x", 
                  phyId1, phyId2, I219V_PHY_ID1_EXPECTED, I219V_PHY_ID2_EXPECTED);
        // Продолжаем инициализацию, так как некоторые версии могут иметь другие ID
    }

    // Чтение статуса PHY
    phyStatus = I219vReadPhy(DeviceContext, I219V_PHY_STATUS);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "PHY Status: 0x%04x", phyStatus);

    // Настройка автосогласования
    phyAna = I219vReadPhy(DeviceContext, I219V_PHY_AUTONEG_ADV);
    
    // Установка поддерживаемых режимов
    phyAna |= I219V_PHY_AUTONEG_ADV_10T_HD |
              I219V_PHY_AUTONEG_ADV_10T_FD |
              I219V_PHY_AUTONEG_ADV_100TX_HD |
              I219V_PHY_AUTONEG_ADV_100TX_FD;
    
    // Установка режима паузы
    phyAna &= ~I219V_PHY_AUTONEG_ADV_PAUSE_MASK;
    phyAna |= I219V_PHY_AUTONEG_ADV_PAUSE_BOTH;
    
    // Запись регистра рекламы автосогласования
    I219vWritePhy(DeviceContext, I219V_PHY_AUTONEG_ADV, phyAna);

    // Настройка 1000BASE-T
    phy1000tCtrl = I219vReadPhy(DeviceContext, I219V_PHY_1000T_CTRL);
    
    // Установка поддержки 1000BASE-T
    phy1000tCtrl |= I219V_PHY_1000T_CTRL_ADV_1000T_FD;
    phy1000tCtrl &= ~I219V_PHY_1000T_CTRL_ADV_1000T_HD;  // Отключение полудуплекса для 1000BASE-T
    
    // Запись регистра управления 1000BASE-T
    I219vWritePhy(DeviceContext, I219V_PHY_1000T_CTRL, phy1000tCtrl);

    // Настройка управления медью
    USHORT copperCtrl = I219vReadPhy(DeviceContext, I219V_PHY_COPPER_CTRL);
    
    // Включение Auto-MDIX
    copperCtrl |= I219V_PHY_COPPER_CTRL_AUTO_MDIX;
    
    // Запись регистра управления медью
    I219vWritePhy(DeviceContext, I219V_PHY_COPPER_CTRL, copperCtrl);

    // Настройка управления PHY
    phyCtrl = I219vReadPhy(DeviceContext, I219V_PHY_CONTROL);
    
    // Включение автосогласования
    phyCtrl |= I219V_PHY_CTRL_AUTONEG;
    
    // Запуск автосогласования
    phyCtrl |= I219V_PHY_CTRL_RESTART_AN;
    
    // Запись регистра управления PHY
    I219vWritePhy(DeviceContext, I219V_PHY_CONTROL, phyCtrl);

    // Настройка Energy Efficient Ethernet (EEE)
    I219vConfigureEee(DeviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "PHY initialized successfully");
    return STATUS_SUCCESS;
}

// Настройка Energy Efficient Ethernet (EEE)
VOID
I219vConfigureEee(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    UINT32 eeer;
    USHORT lpAbility;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Configuring EEE");

    // Чтение возможностей EEE партнера
    lpAbility = I219vReadPhy(DeviceContext, I219V_EEE_LP_ABILITY);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "EEE LP Ability: 0x%04x", lpAbility);

    // Чтение текущего значения регистра EEE
    eeer = I219vReadRegister(DeviceContext, I219V_REG_EEER);

    // Проверка поддержки EEE партнером
    if (lpAbility & (I219V_EEE_100_SUPPORTED | I219V_EEE_1000_SUPPORTED)) {
        // Партнер поддерживает EEE
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "EEE supported by link partner");
        
        // Включение EEE
        eeer |= I219V_EEER_TX_LPI_EN | I219V_EEER_RX_LPI_EN | I219V_EEER_LPI_FC;
    } else {
        // Партнер не поддерживает EEE
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "EEE not supported by link partner");
        
        // Отключение EEE
        eeer &= ~(I219V_EEER_TX_LPI_EN | I219V_EEER_RX_LPI_EN | I219V_EEER_LPI_FC);
    }

    // Запись регистра EEE
    I219vWriteRegister(DeviceContext, I219V_REG_EEER, eeer);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "EEE Register: 0x%08x", eeer);
}

// Получение состояния соединения
VOID
I219vGetLinkState(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _Out_ PNET_ADAPTER_LINK_STATE LinkState
    )
{
    USHORT phyStatus, copperStat;
    UINT32 status;
    NDIS_LINK_SPEED linkSpeed = 0;
    MEDIA_DUPLEX_STATE duplexState = MediaDuplexStateUnknown;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Getting link state");

    // Чтение статуса PHY
    phyStatus = I219vReadPhy(DeviceContext, I219V_PHY_STATUS);
    
    // Чтение статуса меди
    copperStat = I219vReadPhy(DeviceContext, I219V_PHY_COPPER_STAT);
    
    // Чтение статуса устройства
    status = I219vReadRegister(DeviceContext, I219V_REG_STATUS);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "PHY Status: 0x%04x, Copper Status: 0x%04x, Device Status: 0x%08x", 
              phyStatus, copperStat, status);

    // Проверка состояния соединения
    if ((phyStatus & I219V_PHY_STATUS_LINK_UP) && (status & I219V_STATUS_LU)) {
        // Соединение установлено
        
        // Определение скорости соединения
        switch (copperStat & I219V_PHY_COPPER_STAT_SPEED_MASK) {
        case I219V_PHY_COPPER_STAT_SPEED_1000:
            linkSpeed = NDIS_LINK_SPEED_1000MBPS;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Link speed: 1000 Mbps");
            break;
        case I219V_PHY_COPPER_STAT_SPEED_100:
            linkSpeed = NDIS_LINK_SPEED_100MBPS;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Link speed: 100 Mbps");
            break;
        case I219V_PHY_COPPER_STAT_SPEED_10:
            linkSpeed = NDIS_LINK_SPEED_10MBPS;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Link speed: 10 Mbps");
            break;
        default:
            linkSpeed = NDIS_LINK_SPEED_UNKNOWN;
            TraceEvents(TRACE_LEVEL_WARNING, TRACE_HARDWARE, "Unknown link speed");
            break;
        }
        
        // Определение режима дуплекса
        if (copperStat & I219V_PHY_COPPER_STAT_DUPLEX) {
            duplexState = MediaDuplexStateFull;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Duplex: Full");
        } else {
            duplexState = MediaDuplexStateHalf;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Duplex: Half");
        }
        
        // Инициализация состояния соединения
        NET_ADAPTER_LINK_STATE_INIT(
            LinkState,
            linkSpeed,
            MediaConnectStateConnected,
            duplexState,
            NetAdapterPauseFunctionTypeUnsupported,
            NetAdapterAutoNegotiationFlagXmitLinkSpeed |
            NetAdapterAutoNegotiationFlagRcvLinkSpeed |
            NetAdapterAutoNegotiationFlagDuplexMode
        );
    } else {
        // Соединение отсутствует
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Link down");
        NET_ADAPTER_LINK_STATE_INIT_DISCONNECTED(LinkState);
    }
}

// Настройка управления питанием PHY
VOID
I219vConfigurePowerManagement(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext,
    _In_ BOOLEAN EnableWakeOnLan
    )
{
    UINT32 pmcsr, phpm;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Configuring power management, WoL %s", 
              EnableWakeOnLan ? "enabled" : "disabled");

    // Чтение регистра управления питанием
    pmcsr = I219vReadRegister(DeviceContext, I219V_REG_PMCSR);
    
    // Чтение регистра управления питанием PHY
    phpm = I219vReadRegister(DeviceContext, I219V_REG_PHPM);

    if (EnableWakeOnLan) {
        // Включение Wake-on-LAN
        pmcsr |= I219V_PMCSR_PME_EN;
        
        // Настройка PHY для поддержки Wake-on-LAN
        phpm &= ~(I219V_PHPM_SPD_EN | I219V_PHPM_D0A_LPLU | I219V_PHPM_LPLU);
    } else {
        // Отключение Wake-on-LAN
        pmcsr &= ~I219V_PMCSR_PME_EN;
        
        // Включение энергосбережения PHY
        phpm |= I219V_PHPM_SPD_EN;
    }

    // Запись регистра управления питанием
    I219vWriteRegister(DeviceContext, I219V_REG_PMCSR, pmcsr);
    
    // Запись регистра управления питанием PHY
    I219vWriteRegister(DeviceContext, I219V_REG_PHPM, phpm);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "Power management configured, PMCSR: 0x%08x, PHPM: 0x%08x", 
              pmcsr, phpm);
}

// Настройка LED-индикаторов
VOID
I219vConfigureLeds(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    USHORT ledCtrl;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Configuring LEDs");

    // Чтение регистра управления LED
    ledCtrl = I219vReadPhy(DeviceContext, I219V_PHY_LED_CTRL);
    
    // Настройка LED0 (Link/Activity)
    ledCtrl &= ~0x000F;  // Очистка битов LED0
    ledCtrl |= 0x0002;   // LED0 = Link/Activity
    
    // Настройка LED1 (Speed)
    ledCtrl &= ~0x00F0;  // Очистка битов LED1
    ledCtrl |= 0x0010;   // LED1 = 10 Mbps
    
    // Настройка LED2 (Speed)
    ledCtrl &= ~0x0F00;  // Очистка битов LED2
    ledCtrl |= 0x0100;   // LED2 = 100/1000 Mbps
    
    // Настройка режима мигания
    ledCtrl &= ~0xF000;  // Очистка битов режима мигания
    ledCtrl |= 0x1000;   // Стандартный режим мигания
    
    // Запись регистра управления LED
    I219vWritePhy(DeviceContext, I219V_PHY_LED_CTRL, ledCtrl);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, 
              "LEDs configured, LED Control: 0x%04x", ledCtrl);
}

// Диагностика PHY
NTSTATUS
I219vDiagnosticPhy(
    _In_ PI219V_DEVICE_CONTEXT DeviceContext
    )
{
    USHORT phyCtrl, phyStatus, copperStat, copperCtrl;
    USHORT phyId1, phyId2, phy1000tStatus;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "Running PHY diagnostics");

    // Чтение идентификаторов PHY
    phyId1 = I219vReadPhy(DeviceContext, I219V_PHY_ID1);
    phyId2 = I219vReadPhy(DeviceContext, I219V_PHY_ID2);

    // Чтение регистров статуса и управления
    phyCtrl = I219vReadPhy(DeviceContext, I219V_PHY_CONTROL);
    phyStatus = I219vReadPhy(DeviceContext, I219V_PHY_STATUS);
    copperCtrl = I219vReadPhy(DeviceContext, I219V_PHY_COPPER_CTRL);
    copperStat = I219vReadPhy(DeviceContext, I219V_PHY_COPPER_STAT);
    phy1000tStatus = I219vReadPhy(DeviceContext, I219V_PHY_1000T_STATUS);

    // Вывод диагностической информации
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "PHY Diagnostics:");
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  PHY ID: 0x%04x 0x%04x", phyId1, phyId2);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Control: 0x%04x", phyCtrl);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Status: 0x%04x", phyStatus);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Copper Control: 0x%04x", copperCtrl);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Copper Status: 0x%04x", copperStat);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  1000T Status: 0x%04x", phy1000tStatus);

    // Проверка состояния соединения
    if (phyStatus & I219V_PHY_STATUS_LINK_UP) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Link is UP");
        
        // Определение скорости соединения
        switch (copperStat & I219V_PHY_COPPER_STAT_SPEED_MASK) {
        case I219V_PHY_COPPER_STAT_SPEED_1000:
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Speed: 1000 Mbps");
            break;
        case I219V_PHY_COPPER_STAT_SPEED_100:
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Speed: 100 Mbps");
            break;
        case I219V_PHY_COPPER_STAT_SPEED_10:
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Speed: 10 Mbps");
            break;
        default:
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Speed: Unknown");
            break;
        }
        
        // Определение режима дуплекса
        if (copperStat & I219V_PHY_COPPER_STAT_DUPLEX) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Duplex: Full");
        } else {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Duplex: Half");
        }
        
        // Проверка режима MDI/MDIX
        if (copperStat & I219V_PHY_COPPER_STAT_MDIX) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  MDI/MDIX: MDIX");
        } else {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  MDI/MDIX: MDI");
        }
        
        // Проверка состояния автосогласования
        if (phyStatus & I219V_PHY_STATUS_AUTONEG_COMP) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Auto-negotiation: Complete");
        } else {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Auto-negotiation: In progress");
        }
    } else {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_HARDWARE, "  Link is DOWN");
    }

    return STATUS_SUCCESS;
}
