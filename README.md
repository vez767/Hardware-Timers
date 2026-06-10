# HC-SR04 FreeRTOS Driver & DSP Tracking Engine (v1.2.0)

## System Overview
This module provides a Real-Time Operating System (FreeRTOS) driver for the HC-SR04 Ultrasonic Sensor, engineered for the STM32F microcontroller family. It replaces synchronous, CPU-blocking architecture with an asynchronous, event-driven pipeline.

By leveraging hardware timers, external interrupts (EXTI), and RTOS Semaphores, the CPU is completely decoupled from the acoustic flight time. The system actively tracks targets, filters out acoustic anomalies, and guarantees safe states during severe physical hardware faults without freezing the kernel.

>  **Engineering Evolution Note:** This project was originally built as a raw, direct register-mapped tracking engine. To explore the low-level memory mapping foundations, clock calibration records, and logic analyzer verification steps, view the comprehensive [ARCHITECTURE.md](./ARCHITECTURE.md).

---

## Architecture & Key Features
- **Deferred RTOS Processing:** The traditional blocking while loop has been replaced with a FreeRTOS Binary Semaphore (`EchoSemaphore`). The CPU sleeps during the acoustic pulse's flight time and is instantly awakened via a PendSV context switch when the EXTI hardware catches the returning echo.
- **Hardware Stopwatch (TIM2):** Bypasses the HAL to control the STM32 hardware timer directly. It is explicitly calibrated to the 16MHz HSI oscillator (`TIM2_PSC = 15`) to generate a deterministic 10µs trigger pulse.
- **Digital Signal Processing (DSP) Pipeline:**
  - **Dynamic Application Gate:** Enforces a strict 200cm window limit to secure a noise-free data stream.
  - **Slew-Rate Filter:** Evaluates physical velocity between frames, rejecting physically impossible distance spikes (>50cm jumps) caused by acoustic scattering.
  - **Accumulator:** Buffers 8 consecutive valid samples to output a mathematically stable tracking metric.
- **Kernel Protection (Panic Room):** Implements a `vApplicationStackOverflowHook` that bypasses the OS entirely during a stack memory crash. It forces `GPIOA_MODER` to output mode and uses raw timer cycles to strobe a hardware LED, visually guaranteeing fault notification even if the OS is dead.

---

## RTOS Hardware-in-the-Loop (HIL) Demonstrations
The following states detail how the RTOS pipeline behaves under physical testing, translating the original bare-metal state machine into asynchronous OS logic.

> #### Please note, the window tracking limit was increased from 100cm to 200cm.

### Case 1: System Idle (No Target in Range)
- **Status:** Nominal.
- **Behavior:** The sensor fires, but the target is beyond the 200cm Application Gate. The DSP filter rejects the raw data and forcefully outputs the invalidation error code (`11111111`) to prevent downstream systems from acting on ghost data.

### Case 2: Target Acquisition & Tracking
- **Status:** Active Lock.
- **Behavior:** An object enters the envelope. It passes both the Gate and the Slew-Rate checks. The DSP Accumulator gathers 8 valid samples, divides the pulse width, and updates `filtered_distance` with the true physical target position.

### Case 3: Target Lost / Out of Range
- **Status:** Nominal Recovery.
- **Behavior:** The object leaves the 200cm boundary. The DSP instantly trips the Gate logic and overwrites the stale tracking data with the `11111111` safe state code.

### Case 4: Hardware Failure (Severed Wire / Pulled Ground)
- **Status:** Safe State Triggered.
- **Behavior:** The physical ECHO wire is disconnected mid-flight. The EXTI interrupt never fires. Instead of trapping the CPU in an infinite loop, the `xSemaphoreTake` hits its strict 50ms block time. The RTOS task wakes up, acknowledges the hardware timeout, and outputs `11111111`. The rest of the system continues running perfectly.

### Case 5: Hardware Reconnection & Recovery
- **Status:** System Restored.
- **Behavior:** The severed wire is plugged back in. Because the `vSensorTask` is still looping and firing the trigger every 100ms, the very next cycle successfully catches the EXTI interrupt. The semaphore unlocks, and the system seamlessly resumes tracking without requiring a physical reset.

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
To adhere to proper RTOS design patterns, raw global variables for telemetry (such as `sensor_status` and `clean_envelope`) have been temporarily deprecated.

In the upcoming System Telemetry & Safety Branch, these indicators will be resurrected inside a dedicated C struct. This telemetry payload will be safely passed across the RTOS via Queues, and the entire system will be monitored by an Independent Watchdog (IWDG) task to guarantee total system integrity.
