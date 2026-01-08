# ⚙️ Embedded-ControlHub

**Embedded-ControlHub** is a modular multi-MCU embedded system built around a **clear master–slave architecture**, deterministic communication, and real-world integration patterns.

At its core, the system uses **STM32 as a central master controller**, coordinating actuators and sensors distributed across **Arduino UNO** and **ESP32** via **I2C**, while exposing a clean **UART interface** for external control and monitoring.

This project is designed not as a demo sketch, but as a **foundation for real embedded systems**: extensible, observable, and easy to reason about.

---

## 🧠 System Architecture

### Roles & Responsibilities

* **STM32 (Master Controller)**

  * Central logic and orchestration
  * Routes commands and data between subsystems
  * Exposes a UART text interface for operators (PC / Linux)
  * Periodically polls sensor nodes

* **Arduino UNO (Executor / Actuator Node)**

  * I2C slave
  * Executes physical actions
  * Controls motors, relay, and reads local input
  * Reports events back to the master

* **ESP32 (Sensors + Web Node)**

  * I2C slave
  * Collects environmental data
  * Runs a local HTTP server
  * Uses FreeRTOS for parallel task execution

---

## 🔗 Communication Overview

```
PC / Linux
   |
   |  UART (USB CDC)
   v
STM32 (Master)
   |
   |-- I2C 0x08 → Arduino UNO (Actuators)
   |
   |-- I2C 0x09 → ESP32 (Sensors)
```

* All **hardware control** goes through STM32
* Slaves never talk to each other directly
* Protocols are **human-readable** for transparency and debugging

---

## 📂 Repository Structure

All directories are intentionally separated by hardware role.
Folder names use **PascalCase** for clarity.

```
Embedded-ControlHub/
│
├── Arduino/
│   └── Executor/
│       └── Executor.ino
│
├── ESP32/
│   └── SensorsWeb/
│       └── SensorsWeb.ino
│
├── STM32/
│   └── Core/
│       └── Src/
│           └── main.c
│
├── Diagrams/
│   ├── Arduino_Executor.png
│   ├── ESP32_Sensors.png
│   └── System_Overview.png
│
└── README.md
```

---

## 🔧 Arduino UNO — Executor Node

### Purpose

Arduino UNO acts as a **pure executor**.
It does not make decisions — it performs actions requested by the STM32 master.

### Connected Hardware

* Servo motor
* Stepper motor (28BYJ-48 + ULN2003)
* Relay
* Push button

### I2C Command Interface

Arduino accepts **text-based I2C commands**:

| Command             | Description               |
| ------------------- | ------------------------- |
| `SERVO SET <0–180>` | Set servo angle           |
| `STEPPER MOVE <N>`  | Rotate stepper by N steps |
| `RELAY ON`          | Enable relay              |
| `RELAY OFF`         | Disable relay             |
| `STATUS`            | Query current state       |

### Event Reporting

* Button press events are reported back to STM32
* Events are appended to the I2C response (`+BTN PRESSED`)

---

## 🌐 ESP32 — SensorsWeb Node

### Purpose

ESP32 is a **sensor aggregation and networking node**.
It bridges low-level sensor data with higher-level interfaces.

### Sensors

* 🌡 Temperature — DHT11
* 💧 Humidity — DHT11
* 💡 Light level — photoresistor (ADC)
* 👏 Sound — digital clap detection

### Interfaces

* **I2C** — fixed-size packets to STM32
* **HTTP** — JSON API
* **Web UI** — real-time local dashboard

### Concurrency Model

ESP32 uses **FreeRTOS**:

* Sensor reading task
* Sound detection task
* HTTP server task

Shared data is protected with a mutex.

---

## 🧠 STM32 — Master Controller

### Role

STM32 is the **brain of the system**.

It:

* receives commands over UART
* routes commands to Arduino via I2C
* polls ESP32 sensor data periodically
* aggregates and prints responses

### UART Interface

* Baud rate: `115200`
* Protocol: text-based
* Human-readable, terminal-friendly

Example commands:

```
SENSORS
STOP
RELAY ON
RELAY OFF
STEPPER MOVE 512
SERVO SET 90
STATUS
```

---

## 📊 Diagrams

To keep diagrams readable, the system is documented in **three layers**:

1. **Arduino Executor Diagram** — actuators and wiring
2. **ESP32 Sensors Diagram** — sensors, tasks, web
3. **System Overview Diagram** — I2C + UART topology

---

## 💡 Design Principles

* ✅ Clear separation of responsibilities
* ✅ Master–slave architecture
* ✅ Text-based protocols
* ✅ Deterministic data flow
* ✅ Scalable and extensible
* ✅ Linux-friendly by design

---

## 🧪 Future Extensions

* Add CRC to I2C packets
* Persistent logging
* MQTT / CAN integration
* Watchdog & fault recovery

---

> **Embedded-ControlHub** is built as a serious embedded systems project —
> focused on architecture, clarity, and real-world workflows.

⚙️ Multi-MCU • 📡 Deterministic • 🧠 Clean Architecture
