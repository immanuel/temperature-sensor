/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include <stdio.h>
#include <math.h>

uint8 adc_state;

#define ADC_STATE_ON 1
#define ADC_STATE_OFF 0

#define WDT_DELAY_32_SEC 20
#define WDT_DELAY_16_SEC 19
#define WDT_DELAY_8_SEC 18
#define WDT_DELAY_4_SEC 17
#define WDT_DELAY_2_SEC 16
#define WDT_DELAY_1_SEC 15
#define WDT_DELAY_500_MS 14
#define WDT_DELAY_250_MS 13
#define WDT_DELAY_125_MS 12
#define WDT_DELAY_63_MS 11
#define WDT_DELAY_31_MS 10
#define WDT_DELAY_16_MS 9
#define WDT_DELAY_8_MS 8

float l, temp;
float a = 3.354016E-03, b =  2.569850E-04, c = 2.620131E-06, d =  6.383091E-08;
int16 temp_int16;

int16 adc_result;

extern CYBLE_GAPP_DISC_MODE_INFO_T cyBle_discoveryModeInfo;
#define MANUFACTURER_SPECIFIC_DYNAMIC_DATA_INDEX    (7u) /* index of the dynamic data in the ADV packet */

void WatchdogTimer_Isr(void)
{
    //CySysWdtDisable(CY_SYS_WDT_COUNTER2_MASK);
    //CySysWdtResetCounters(CY_SYS_WDT_COUNTER2_RESET);
    adc_state = ADC_STATE_ON;
    sensor_pwr_Write(1);
    ADC_Wakeup();
    ADC_Enable();
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    
    adc_result = ADC_GetResult16(0);
    l = log(1.0 * adc_result / (2048-adc_result));
    temp = 1.0 / (a + b*l + c*l*l + d*l*l*l*l) - 273.15;
    temp_int16 = (int16)(temp*100);
    
    sensor_pwr_Write(0);
    ADC_Sleep();
    adc_state = ADC_STATE_OFF;

    //CySysWdtEnable(CY_SYS_WDT_COUNTER2_MASK);
    //CySysWdtClearInterrupt(reason);
    CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER2_INT);    
}

void CustomEventHandler(uint32 event, void * eventParam)
{
    CYBLE_BLESS_CLK_CFG_PARAMS_T clockConfig;
    
	switch(event)
    {
        case CYBLE_EVT_STACK_ON:
            /* C8. Get the configured clock parameters for BLE subsystem */
            CyBle_GetBleClockCfgParam(&clockConfig);
            /* C8. Set the device sleep-clock accuracy (SCA) based on the tuned ppm
            of the WCO */
            clockConfig.bleLlSca = CYBLE_LL_SCA_000_TO_020_PPM;
            /* C8. Set the clock parameter of BLESS with updated values */
            CyBle_SetBleClockCfgParam(&clockConfig);
            
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
			/* Start Advertisement and enter Discoverable mode*/
			CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
			break;
			
		case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
			/* Set the BLE state variable to control LED status */
            if(CYBLE_STATE_DISCONNECTED == CyBle_GetState())
            {
                /* Start Advertisement and enter Discoverable mode*/
                CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            }
			break;
        
        default:

       	 	break;
    }
}

void ManageSystemPower()
{
    /* Variable declarations */
    CYBLE_BLESS_STATE_T blePower;
    uint8 interruptStatus ;
    /* Disable global interrupts to avoid any other tasks from interrupting this
    section of code*/
    interruptStatus  = CyEnterCriticalSection();
    /* C9. Get current state of BLE sub system to check if it has successfully
    entered deep sleep state */
    blePower = CyBle_GetBleSsState();
    /* C10. System can enter Deep-Sleep only when BLESS and rest of the application
    are in DeepSleep or equivalent power modes */
    if((blePower == CYBLE_BLESS_STATE_DEEPSLEEP || blePower == CYBLE_BLESS_STATE_ECO_ON) && adc_state == ADC_STATE_OFF)
    {
        /* C11. Put system into Deep-Sleep mode*/
        CySysPmDeepSleep();
    }
    /* C12. BLESS is not in Deep Sleep mode. Check if it can enter Sleep mode */
    else if((blePower != CYBLE_BLESS_STATE_EVENT_CLOSE))
    {
        /* C13. Application is in Deep Sleep. IMO is not required */
        if(adc_state == ADC_STATE_OFF)
        {
        /* C14. change HF clock source from IMO to ECO*/
        CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_ECO);
        /* C14. stop IMO for reducing power consumption */
        CySysClkImoStop();
        /*C15. put the CPU to sleep */
        CySysPmSleep();
        /* C16. starts execution after waking up, start IMO */
        CySysClkImoStart();
        /* C16. change HF clock source back to IMO */
        CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_IMO);
        }
        /*C17. Application components need IMO clock */
        else if(adc_state == ADC_STATE_ON)
        {
            /* C18. Put the system into Sleep mode*/
            // TODO: Does ADC require system active?
            CySysPmSleep();
        }
    }
    /* Enable interrupts */
    CyExitCriticalSection(interruptStatus );
}

int main(void)
{
    ADC_Start();
    ADC_Sleep();
    adc_state = ADC_STATE_OFF;
     /* Enable global interrupts. */
    CyGlobalIntEnable;
    
    //CySysWdtUnlock(); // Enable modification of the WDT Timers
    //CySysWdtDisable(CY_SYS_WDT_COUNTER2_MASK);
    CySysWdtSetMode(2, CY_SYS_WDT_MODE_INT);
    CySysWdtSetToggleBit(WDT_DELAY_16_SEC);
    
    /* Setup ISR for interrupts at WDT counter 0 events. */
    CySysWdtSetInterruptCallback(2,WatchdogTimer_Isr); 
    CySysWdtEnable(CY_SYS_WDT_COUNTER2_MASK);
    CySysWdtEnableCounterIsr(2);
    
    
    CyBle_Start(CustomEventHandler);
    
    /* Set XTAL divider to 3MHz mode */
    CySysClkWriteEcoDiv(CY_SYS_CLK_ECO_DIV8); 
    
    /* ILO is no longer required, shut it down */
    CySysClkIloStop();
    for(;;)
    {
        CyBle_ProcessEvents();
        
        if(CyBle_GetBleSsState() == CYBLE_BLESS_STATE_EVENT_CLOSE)
        {
            (cyBle_discoveryModeInfo.advData->advData)[MANUFACTURER_SPECIFIC_DYNAMIC_DATA_INDEX] = temp_int16;
            (cyBle_discoveryModeInfo.advData->advData)[MANUFACTURER_SPECIFIC_DYNAMIC_DATA_INDEX+1] = (temp_int16 >> 8);
            CyBle_GapUpdateAdvData(cyBle_discoveryModeInfo.advData, cyBle_discoveryModeInfo.scanRspData);
            
        }
        
        CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);
        
        ManageSystemPower();
        
    }
}

/* [] END OF FILE */
