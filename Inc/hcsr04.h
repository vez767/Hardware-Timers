/*
 * hcsr04.h
 *
 *  Created on: 29 May 2026
 *      Author: vez767
 */

#ifndef HCSR04_H_
#define HCSR04_H_

#include <stdint.h>

#define RCC_BASE		(0x40023800UL)
#define GPIOA_BASE		(0x40020000UL)
#define SYSCFG_BASE		(0x40013800UL)
#define EXTI_BASE		(0x40013C00UL)
#define NVIC_IPR_BASE   (0xE000E400UL)
#define NVIC_ISER_BASE	(0xE000E100UL)
#define TIM2_BASE		(0x40000000UL)

#define RCC_AHB1ENR 	(*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR 	(*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB2ENR 	(*(volatile uint32_t *)(RCC_BASE + 0x44))

#define GPIOA_MODER		(*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_PUPDR		(*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define GPIOA_IDR		(*(volatile uint32_t *)(GPIOA_BASE + 0x10))
#define GPIOA_ODR		(*(volatile uint32_t *)(GPIOA_BASE + 0x14))

#define SYSCFG_EXTICR1	(*(volatile uint32_t *)(SYSCFG_BASE + 0x08))

#define EXTI_IMR 		(*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_FTSR 		(*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_RTSR 		(*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_PR			(*(volatile uint32_t *)(EXTI_BASE + 0x14))


#define NVIC_IPR1		(*(volatile uint32_t *)(NVIC_IPR_BASE + 0x04))
#define NVIC_ISER0 		(*(volatile uint32_t *)(NVIC_ISER_BASE + 0x00))

#define TIM2_EGR		(*(volatile uint32_t *)(TIM2_BASE + 0x14))
#define TIM2_PSC		(*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_CNT		(*(volatile uint32_t *)(TIM2_BASE + 0x24))
#define TIM2_ARR		(*(volatile uint32_t *)(TIM2_BASE + 0x2C))
#define TIM2_CR1		(*(volatile uint32_t *)(TIM2_BASE + 0x00))

void HCSR04_Init(void);
void Trig_Set(uint8_t set_time_us);



#endif /* HCSR04_H_ */
