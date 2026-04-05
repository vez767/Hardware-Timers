# HC-SR04 Direct-Register Driver & Fault-Tolerant State Machine

## System Overview
This module provides a direct-register hardware driver for the HC-SR04 Ultrasonic Sensor, specifically engineered for the STM32F microcontroller family. By intentionally bypassing the Hardware Abstraction Layer (HAL), the architecture maintains absolute control over MCU registers, microsecond timing, and memory allocation. 

While the engine is optimized for high-fidelity execution under nominal conditions, it is explicitly designed to anticipate and survive severe physical hardware faults. Unlike standard, standalone ultrasonic scripts, this module is built to exemplify integration-ready component for larger safety-critical systems. It features automated reconnection logic, ensuring that if a circuit is physically severed and later restored, the state machine recovers seamlessly without requiring a system reboot.

Moving beyond basic static distance measurement, this driver incorporates an active dynamic tracking engine. By establishing a strictly tested operational envelope, the system locks onto a moving target and performs continuous slew-rate calculations. This allows the MCU to actively filter out acoustic noise, reject impossible distance spikes, and explicitly report the sensor's health and telemetry status to the user.

---

## Architecture & Key Features

* **Hardware Initialization (TIM2):** Configured the STM32F's TIM2 hardware timer to generate microsecond-resolution time bases and control the sensor's PWM trigger pulse. By explicitly bypassing the standard HAL libraries, the driver maintains direct, low-latency control over the MCU registers.
* **Fault Tolerance & Diagnostics (Dual Timeouts):** Transformed critical hardware failure points into actionable diagnostic data. By implementing dual microsecond timeouts for both the rising and falling edges, the system prevents CPU lockup and explicitly reports exactly which phase of the acoustic cycle failed to the user.
* **Signal Processing (Dynamic Application Gate):** Configured an adaptable, deterministic application range gate to define a functional detection boundary. Active immediately upon initialization, this gate secures a noise-free data stream and prevents initial false-locks.
* **Target Tracking (Slew-Rate Filter):** Upgraded from static boundaries to a dynamic `last_contact` state tracker. By enforcing physical slew-rate limitations, the algorithm rejects physically impossible distance spikes caused by sensor hardware limits or external acoustic noise, ensuring smooth and realistic target tracking.
* **Optimized Polling Engine:** Tuned the state machine to execute an 18-iteration polling loop. This specific constraint was physically tested and verified as the optimal balance between real-time target sensitivity and overarching system update speed.
* **System Safety (Stale Data Invalidation):** Integrated safety-critical fallbacks to aggressively invalidate expired telemetry. Upon target loss or a broken circuit (which otherwise freezes the hardware capture), the system immediately overwrites the stale memory with an explicit `1111111` error code. This guarantees that neither the end user nor downstream systems will ever rely on or execute commands based on ghost data.

---

## Hardware-in-the-Loop (HIL) Demonstrations



> https://github.com/user-attachments/assets/dfabf98f-dbc6-4247-bb96-9f3e8f0d15e7



The following test cases break down the specific system states demonstrated in the hardware testing. They track the dynamic calculation of pulse widths alongside the real-time evaluation of the safe-state variables (`raw_distance_cm`, `filtered_distance`, `sensor_status`, `trigger_timeout`, and `clean_envelope`).

### Case 1: System Idle (No Target in Range)
##### **Status:** Nominal
The sensor is functioning normally (`sensor_status = 1`), but no object is within the defined window limit. The `clean_envelope` flag remains high (`1`), and the `filtered_distance` correctly outputs the invalidation code (`1111111`) to indicate no valid target is present.
> <img width="1920" height="1080" alt="Screenshot 2026-04-04 143721" src="https://github.com/user-attachments/assets/744d4ced-0f53-4fd9-8143-a1ab66c26f43" />


### Case 2: Target Acquisition & Tracking
##### **Status:** Active Lock
An object enters the window limit. The `clean_envelope` flag drops to `0` indicating an active lock, and the `filtered_distance` dynamically updates with the true distance of the target in real-time.
> <img width="1920" height="1080" alt="Screenshot 2026-04-04 143656" src="https://github.com/user-attachments/assets/992c80af-487d-4264-9015-0f401ef19f9b" />


### Case 3: Target Lost / Out of Range
##### **Status:** Nominal Recovery
The tracked object moves out of the physical range limit. The system safely drops the lock, returning `clean_envelope` to `1` and actively invalidating the stale tracking data by resetting `filtered_distance` back to `1111111`.

### Case 4: Hardware Failure (Severed Echo Wire) or Faulty Connection 
##### **Status:** Safe State Triggered
In two seperate instances the circuit is broken: A faulty connection where connections for Port A0 and A1 are mixed up and a scenerio where Echo wire is physically unplugged while the system is running. The hardware capture freezes, but the `trigger_timeout` variable dynamically counts up to the `50000` fail-safe limit. The system successfully intercepts the fault, sets `sensor_status` to `0`, and outputs the `1111111` invalidation code. Crucially, the polling timer does not halt; it continuously attempts new trigger cycles, actively re-testing the connection to check if the hardware has returned.
> <img width="1919" height="1079" alt="Screenshot 2026-04-04 143535" src="https://github.com/user-attachments/assets/608e1446-9c7c-41ba-977f-f2a365a6d1d3" />


### Case 5: Hardware Reconnection & Recovery
**Status:** System Restored
Because the timer continuously polls the hardware during a fault (as seen in Case 4), plugging the severed Echo wire back in allows the very next cycle to complete successfully. The system automatically catches the recovery, updates `sensor_status` back to `1`, resets the `trigger_timeout` to steady-state, and resumes idle monitoring without requiring a system reboot.


---

## Pinout & Hardware Configuration

This driver is mapped to the standard GPIO Port A of the STM32F microcontroller. 

| HC-SR04 Pin | STM32F Pin | Configuration / Logic |
| :--- | :--- | :--- |
| **VCC** | 3.3V | Main Power Supply |
| **GND** | GND | Common Ground |
| **TRIG** | Port - A0 | Output (TIM2 PWM / GPIO) |
| **ECHO** | Port - A1 | Input (Floating / Pulled-Down) |

### Clock Calibration & Logic Analyzer Verification

Bypassing the HAL requires manual configuration of the MCU's clock tree. While the STM32F family advertises maximum core clocks (e.g., 84MHz), the silicon boots by default onto the **16MHz High-Speed Internal (HSI) oscillator**.

* **Prescaler Calibration:** An initial assumption of an 84MHz system clock led to an incorrect TIM2 prescaler calculation, resulting in a 625ms duty cycle instead of the required microsecond resolution. This was rectified by recalculating the prescaler strictly for the default 16MHz HSI.

> <img width="1917" height="1001" alt="Screenshot 2026-03-30 161449" src="https://github.com/user-attachments/assets/3cb067ee-5b56-4916-a728-3476fbe189a7" />
  
* **Oscillator Drift Compensation:** When targeting a strict 10µs trigger pulse (`CCR1 = 10`), hardware verification revealed the HSI was running slightly faster than its nominal rating, shortening the physical pulse. To compensate for this silicon variance, the capture/compare register was actively tuned to `12U`, outputting a physical pulse of 11.875µs (accounting for the HSI running ~1% faster) to ensure the trigger remained safely above the required threshold.

These timing constraints and clock drifts were physically verified and calibrated using an 8-Channel USB Logic Analyzer tapped directly into the transmission lines.



> 
> <img width="1919" height="955" alt="Screenshot 2026-04-05 144831" src="https://github.com/user-attachments/assets/c18572a8-697b-44dc-ac59-0dd60316c6a9" />

---

## Limitations & Future Improvements

**Acoustic Material Limitations:** As an ultrasonic sensor, the HC-SR04 relies entirely on acoustic echolocation. Testing the tracking gate with soft, porous, or irregular materials (such as a human hand or clothing) will result in inconsistent data or dropped locks due to acoustic absorption and wave scattering. For reliable tracking, hard, flat-surface targets(e.g. hard-cover notebook) are strongly recommended to ensure a clean acoustic reflection.

**Synchronous Blocking Loops:**
While this driver guarantees safety and fault tolerance, the current architecture relies on synchronous blocking loops. Even with the dual microsecond timeouts preventing permanent CPU lockup, the MCU is actively trapped in a `while` loop waiting for the acoustic echo to return. In a complex, multi-sensor environment, this blocks other critical systems from executing.

**Future Architecture Migration:**
As part of the broader system roadmap, this bare-metal logic will be migrated into a Real-Time Operating System (**FreeRTOS**). 
* The blocking `while` loops will be replaced with **Hardware Interrupts (EXTI)**.
* Instead of the CPU actively waiting for the echo, the RTOS Task Scheduler will put the sensor thread to sleep, freeing the CPU to process other systems (like I2C displays or GPS parsing) until the physical `PA1` pin triggers a hardware interrupt to wake it back up.
