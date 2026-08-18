/*
 * hcrs04.c
 *
 *  Created on: 29 May 2026
 *      Author: vez767
 */

#include <stdint.h>
#include <stdlib.h>

#include "hcsr04.h"
#include "iwdg.h"
#include "telemetry.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"




#define SENSOR_TASK_BIT (1 << 0)

extern EventGroupHandle_t IWDG_eventgroup;
volatile UBaseType_t sensor_watermark;
extern QueueHandle_t Distance_Data_Queue;
extern QueueHandle_t Sensor_Payload_Queue;

volatile SensorData_t debug_telemetry;

void HCSR04_Init(void){
						/*GPIO Config*/
	RCC_AHB1ENR |= (1U << 0);

	//PA0 & PA1
	GPIOA_MODER &= ~((3U << 0) | (3U << 2));
	GPIOA_MODER |= (1U << 0);

	GPIOA_PUPDR &= ~(3U << 2);
	GPIOA_PUPDR |= (2U << 2); // Pull-down


						/*ISR Config*/
	RCC_APB2ENR |= (1U << 14); // SYSCFG EN
	SYSCFG_EXTICR1 &= ~(0xFU << 4); // Port A - [0:0:0:0]

	EXTI_IMR |= (1U << 1);

	EXTI_RTSR |= (1U << 1);
	EXTI_FTSR |= (1U << 1);

	NVIC_IPR1 &= ~(0xFFU << 24);
	NVIC_IPR1 |= ((5U << 4) << 24);
	NVIC_ISER0 |=  (1U << 7);


						/*TIM2 Config*/
	RCC_APB1ENR |= (1U << 0);

	TIM2_PSC = 15U;
	TIM2_ARR = 0xFFFFFFFF;
	TIM2_EGR |= (1 << 0);

	TIM2_CR1 |= (1 << 0);
}


void Trig_Set(uint8_t set_time_us){

	GPIOA_ODR |= (1U << 0);
	uint32_t start_time = TIM2_CNT;

	while((TIM2_CNT - start_time) < set_time_us){

	}

	GPIOA_ODR &= ~(1 << 0);

}



uint32_t calc_distance(uint32_t distance){


	static int32_t last_contact = 0;
	static uint8_t valid_samples = 0;
	static uint8_t total_samples = 0;
	static uint32_t distance_accumulator = 0;


	total_samples++;

				if(distance <= 200){   // 200cm - Window Limit
						if(valid_samples == 0 || (abs(last_contact - distance)) < 50){

					last_contact = distance;
					distance_accumulator += distance;
					valid_samples++;

				}
			}




	if(valid_samples == 8){
		uint32_t avg_distance = (uint32_t)(distance_accumulator / 8);

		distance_accumulator = 0;
		valid_samples = 0;
		total_samples = 0;

		return avg_distance;
	}

	if (total_samples >= 10){

		distance_accumulator = 0;
		valid_samples = 0;
		total_samples = 0;

		return 999;
	}

	return 0xFFFFFFFF;
}



void vSensorTask(void *pvParameters){
	uint32_t raw_distance = 0;
	uint32_t result = 0;
	uint32_t received_pulse_width;

	SensorData_t sensor_payload;
	while(1){
		sensor_watermark = uxTaskGetStackHighWaterMark(NULL);

		Trig_Set(10);
		if(xQueueReceive(Distance_Data_Queue, &received_pulse_width, pdMS_TO_TICKS(50))== pdPASS){

			raw_distance = received_pulse_width / 58;
			result = calc_distance(raw_distance);

			if(result != 0xFFFFFFFF){
				if(result == 999){
				sensor_payload.distance = 999;
				sensor_payload.status = 1; // 1 = Clean Envelope

				}else{
			    	sensor_payload.distance = result;
			    	sensor_payload.status = 0; // Object Detected
				}

				xQueueSend(Sensor_Payload_Queue, &sensor_payload, 0);
				debug_telemetry = sensor_payload;
			}

		}else{
			sensor_payload.distance = 0xA98AC7;
			sensor_payload.status = 2; // Hardware timeout

			xQueueSend(Sensor_Payload_Queue, &sensor_payload, 0);
			debug_telemetry = sensor_payload;
			}

	/*	// --- THE CRASH TRAP (Must be here for the test!) ---
				if(sensor_payload.status == 2){
					// If the wire is pulled, freeze the task.
					// The Watchdog Task starves  and the STM32 resets.
					while(1){
						// Trapped!
					}
				}												*/


				xEventGroupSetBits(IWDG_eventgroup, SENSOR_TASK_BIT);
					vTaskDelay(pdMS_TO_TICKS(100));

	}
}

void SensorTask_Init(void){
	 xTaskCreate(vSensorTask, "Sensor", 128, NULL, 1, NULL);
}
