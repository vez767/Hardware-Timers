/*
 * iwdg.c
 *
 *  Created on: 13 Aug 2026
 *      Author: Windows
 */

#include "stm32f4xx.h"

#include "iwdg.h"

#include "FreeRTOS.h"
#include "event_groups.h"

#define SENSOR_TASK_BIT (1 << 0)
extern EventGroupHandle_t IWDG_eventgroup;

void IWDG_Init(void){
	IWDG->KR = 0xCCCC;
	IWDG->KR = 0x5555;

	IWDG->PR &= ~(7U << 0);
	IWDG->PR |= (3U << 0);

	IWDG->RLR = 0x00000FFF;

	while((IWDG->SR & (IWDG_SR_RVU | IWDG_SR_PVU)) != 0){
		//waiting for RVU AND PVU to be cleared by hardware
	}

}

void IWDG_Feed(void){

    IWDG->KR = 0xAAAA;
}

/*
 * Note: This vWatchdogTask architecture and FreeRTOS Event Group logic was co-authored with Gemini AI as a learning reference for
 * centralized Watchdog management. It implements a many-to-one failsafe to monitor system-wide task health.
 */

void vWatchDogTask(void *pvParameters){
	const TickType_t xWatchdogTimeout = pdMS_TO_TICKS(3000); //3 seconds
	EventBits_t uxBits;

	while(1){
		// Wait for the Sensor Task to check in.
		// pdTRUE (3rd param) = Clear bits before returning so we are ready for the next round.
		// pdFALSE (4th param) = Wait for ALL bits (though we only have one right now).
		uxBits = xEventGroupWaitBits(IWDG_eventgroup, SENSOR_TASK_BIT,pdTRUE, pdFALSE, xWatchdogTimeout);

		if( (uxBits & SENSOR_TASK_BIT) == SENSOR_TASK_BIT){
			IWDG_Feed();
		}else{
			// A task froze or crashed!
			// We intentionally skip IWDG_Feed().
			// The hardware will reboot the system in ~1 second.
		}
	}
}

void WatchDogTask_Init(void){
	xTaskCreate(vWatchDogTask, "vWatchDogTask", 128, NULL,configMAX_PRIORITIES - 1, NULL);
}
