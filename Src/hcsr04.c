/*
 * hcrs04.c
 *
 *  Created on: 29 May 2026
 *      Author: vez767
 */

#include <stdint.h>
#include "hcsr04.h"

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

	TIM2_EGR |= (1 << 0);
	TIM2_PSC = 15U;
	TIM2_ARR = 0xFFFFFFFF;

	TIM2_CR1 |= (1 << 0);
}

void Trig_Set(uint8_t set_time_us){

	GPIOA_ODR |= (1U << 0);
	uint32_t start_time = TIM2_CNT;

	while((TIM2_CNT - start_time) < set_time_us){

	}

	GPIOA_ODR &= ~(1 << 0);

}


