/*
 * iwdg.h
 *
 *  Created on: 13 Aug 2026
 *      Author: Windows
 */

#ifndef IWDG_H_
#define IWDG_H_

#include "stm32f4xx.h"

void IWDG_Init(void);
void IWDG_Feed(void);

void vWatchDogTask(void *pvParameters);
void WatchDogTask_Init(void);

#endif /* IWDG_H_ */
