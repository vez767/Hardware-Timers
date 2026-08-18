
https://github.com/user-attachments/assets/d7af9c61-6ff2-407c-8e48-ad446c88de1e
# HC-SR04 FreeRTOS Driver & IPC Telemetry Engine (v1.3.0)

## System Overview
This module provides a Real-Time Operating System (FreeRTOS) driver for the HC-SR04 Ultrasonic Sensor, engineered for the STM32F microcontroller family. Transitioning from a primitive, corruptible global variable IPC architecture, this system upgrades to thread-safe telemetry structs. Additionally, FreeRTOS Event Groups were introduced to prevent task deadlocks and stop the system from hanging.

The system migrates from CPU-blocked flight time calculations to an autonomous ISR and EXTI-driven architecture. It actively tracks targets, filters out acoustic anomalies, and guarantees ISO 14971-compliant safe states during physical hardware faults—all actively monitored by a centralized Independent Watchdog (IWDG).

> **Engineering Note:** This project was originally built as a raw, direct register-mapped tracking engine. To explore the low-level memory mapping foundations, clock calibration records, and logic analyzer verification steps, view the comprehensive [ARCHITECTURE.md](./ARCHITECTURE.md).

---

## Architecture & Key Features
- **Thread-Safe IPC (Queues & Structs):** Global variables and binary semaphores have been eradicated. Hardware timer counts (`TIM2_CNT`) are routed directly from the `EXTI1` interrupt to the Sensor Task via FreeRTOS Queues. Output data is sent through the `Sensor_Payload_Queue` using a `SensorData_t` struct (`telemetry.h`) to securely separate the distance measurement from hardware status codes.
- **Independent Watchdog (IWDG) Event Group:** The `vSensorTask` is monitored by a centralized Watchdog task. Using FreeRTOS Event Groups, the sensor task must remain active and set its event bit every 100ms to meet the IWDG feed requirement. If the task deadlocks, the hardware Watchdog physically reboots the MCU.
- **Hardware Stopwatch (TIM2):** Bypasses the HAL to control the STM32 hardware timer directly using standardized CMSIS header syntax. Explicitly calibrated to the 16MHz HSI oscillator (`TIM2_PSC = 15`) to generate a deterministic 10µs trigger pulse.
- **Digital Signal Processing (DSP) Pipeline:**
  - **Dynamic Application Gate:** Enforces a strict 200cm window limit to secure a noise-free data stream.
  - **Slew-Rate Filter:** Evaluates physical velocity between frames, rejecting physically impossible distance spikes (>50cm jumps) caused by acoustic scattering.
  - **Accumulator:** Buffers 8 consecutive valid samples to output a mathematically stable tracking metric.
- **Kernel Protection (Panic Room):** Implements a `vApplicationStackOverflowHook` that bypasses the OS entirely during a stack memory crash. It forces `GPIOA_MODER` to output mode and uses raw timer cycles to strobe a hardware LED, visually guaranteeing fault notification even if the OS is dead.

---

## RTOS Hardware-in-the-Loop (HIL) Demonstrations
The telemetry pipeline outputs a strict status code hierarchy: Status `0` (Object Detected), Status `1` (Clean Envelope / Noise Filtered), and Status `2` (Hardware Fault). The following states detail how the IPC pipeline behaves under physical testing.

> **Video 1: Live Telemetry & Safe States**
> *(Demonstrates the driver outputting clean distances, switching to Status 1 (`999`) on a clean envelope , and jumping to Status 2 (`11111111`) on physical disconnect.)*
> <video src="https://github.com/user-attachments/assets/7c05b142-9009-4b6b-ab9d-eb1672df2911" width="600" controls></video>

### Case 1: System Idle / Clean Envelope
- **Status:** Nominal (Status `1`).
- **Behavior:** The target is beyond the 200cm Application Gate, or environmental noise causes the DSP to reject 10 consecutive samples. The system refuses to output ghost data. It forces the payload to `999` and flags Status `1`.

### Case 2: Target Acquisition & Tracking
- **Status:** Active Lock (Status `0`).
- **Behavior:** An object enters the envelope. It passes both the Gate and the Slew-Rate checks. The DSP Accumulator gathers 8 valid samples, calculates true physical distance, and securely pushes the `SensorData_t` payload to the Output Queue.

### Case 3: Hardware Failure (Severed Signal Wire)
- **Status:** Safe State Triggered (Status `2`).
- **Behavior:** The physical ECHO wire is disconnected during operation. The EXTI interrupt never fires. Instead of trapping the CPU, the `xQueueReceive` hits its strict 50ms block time. The task wakes up, logs the `0xA98AC7` (`11111111`) hexadecimal fault code, and pushes Status `2` to the system bus. The rest of the MCU continues running perfectly.

> **Video 2: Hardware Reconnection & Non-Blocking Proof**
> *(Demonstrates the system surviving a severe connection loss—such as severing and restoring the ground pin—without hanging, proving the RTOS architecture remains unblocked.)*
> <video src="https://github.com/user-attachments/assets/fd28b106-dec5-4d1d-aef6-468fa08c885a" width="600" controls></video>

### Case 4: Hardware Reconnection & Recovery
- **Status:** System Restored.
- **Behavior:** The severed connection is restored. Because the `vSensorTask` is still looping and firing the trigger every 100ms, the very next cycle successfully catches the EXTI interrupt. The queue receives the pulse, and the system seamlessly resumes tracking without requiring a physical reset.

> **Video 3: Task Freeze & IWDG Reset**
> *(Demonstrates the system booting, triggering an intentional crash trap upon wire pull, followed by the Watchdog forcing a hard reset.)*
> <video src="Uploading HIL Verification - IWDG.mp4…" width="600" controls></video>

### Case 5: Task Freeze (IWDG Hardware Reset)
- **Status:** Watchdog Hard Fault.
- **Behavior:** To validate system-wide fail-safes, a deliberate software trap was activated inside the driver upon a Status `2` fault. When the wire was pulled, the Sensor Task deadlocked. It stopped sending the `SENSOR_TASK_BIT` to the FreeRTOS Event Group. The Watchdog Task starved, and 3 seconds later, the independent silicon Watchdog physically reset the entire STM32 microcontroller.

### Case 6: Kernel Memory Crash (Stack Overflow Hook)
- **Status:** Hardware Exception Triggered.
- **Behavior:** The system's memory allocation was actively profiled using `uxTaskGetStackHighWaterMark()`. Under nominal conditions, the sensor task maintains a safe margin of 78 words *(see attached screenshot)*. To physically validate the safety architecture, the memory allocation was intentionally starved to 28 words. The resulting hardware exception frame overflowed the stack boundary. The RTOS instantly bypassed scheduling, trapped the CPU, and executed the `TIM2` sequence to flash the `PA5` LED as a persistent visual alarm.


<img width="727" height="77" alt="Screenshot 2026-08-16 160133" src="https://github.com/user-attachments/assets/ebdd3c3c-d684-4b6d-95ca-589f75dfb8fd" />

> <video src="https://github.com/user-attachments/assets/5ed952c8-b799-4aa4-b45e-c8925d4ff498" width="600" controls></video>

---

## Pinout & Hardware Configuration

| HC-SR04 Pin | STM32F Pin | Configuration / Logic |
| :--- | :--- | :--- |
| **VCC** | 3.3V / 5V | Main Power Supply |
| **GND** | GND | Common Ground |
| **TRIG** | Port A0 | Output (TIM2 PWM / GPIO) |
| **ECHO** | Port A1 | Input (EXTI Line 1, Both Edges) |
| **LED** | Port A5 | Output (Emergency Panic Room Strobe) |

---

## Future Architecture Migration
With the isolated driver perfected and communicating via standardized RTOS Queues and `SensorData_t` structs, it is ready for master integration. 

The immediate next step in the curriculum is **Tier 3 System Integration**, where this acoustic driver will run concurrently alongside an MPU-6050, DHT22, and ADC DMA engine. The centralized IWDG Event Group will be scaled to monitor all system threads simultaneously using strict AND logic (`pdTRUE`) to ensure comprehensive device health before routing telemetry data to the I2C LCD matrices.
