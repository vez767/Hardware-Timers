/*
 * telemetry.h
 *
 *  Created on: 16 Aug 2026
 *      Author: Windows
 */

#ifndef TELEMETRY_H_
#define TELEMETRY_H_

typedef struct{
	uint32_t distance;
	uint8_t status;		// STATUS CODE: 0 - Object Detected, 1 - Clean Envelope, 2 - Hardware Fault

} SensorData_t;

#endif /* TELEMETRY_H_ */
