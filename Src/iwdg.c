/*
 * iwdg.c
 *
 *  Created on: 13 Aug 2026
 *      Author: Windows
 */

#include "iwdg.h"
#include "stm32f4xx.h"

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
