# 🐧 Linux Integration — Embedded-ControlHub

This directory contains **Linux-side tools** for interacting with the **Embedded-ControlHub** system.

Linux acts as a **monitoring, logging, and operator interface layer**, communicating with the STM32 master controller via **UART (USB CDC)** and directly querying the ESP32 via **HTTP**.

---

## 📦 Contents

| File                | Description                                        |
| ------------------- | -------------------------------------------------- |
| `serial_console.py` | Interactive serial console for STM32 communication |
| `esp32_logger.sh`   | systemd-enabled HTTP logger for ESP32 sensor data  |
| `esp32_logger_worker.sh` | Background worker for polling ESP32 sensors via HTTP |
| `sensors.txt`       | Logged sensor data from STM32 (UART)               |
| `sensors.log`       | Logged sensor data from ESP32 (HTTP)               |

---

## 🧠 Architecture Context

In the overall system:

* **STM32** — Master controller (logic, aggregation, routing)
* **Arduino UNO** — Actuator executor (servo, stepper, relay)
* **ESP32** — Sensor node + HTTP server
* **Linux** — Operator & logging layer *(this directory)*

Linux **does not control hardware directly** — it:

* sends **high-level commands** to STM32
* logs **structured sensor data**
* runs long-lived services (systemd)

---

## 🖥️ serial_console.py

A **cross-platform Python serial console** for communicating with the STM32 master.

### 🔧 Features

* Interactive command-line interface
* Sends text commands to STM32 via UART
* Displays responses in real time
* Logs incoming sensor data to a file
* Works on **Linux and Windows**

---

### 📡 Communication

* Interface: `UART / USB CDC`
* Baud rate: `115200`
* Protocol: **human-readable text**

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

### 🧪 Example Output

```
>>> SENSORS
📡 Sensors enabled
>>> STOP
🛑 Sensors stopped
```

Logged to file:

```
YYYY-MM-DD HH:MM:SS T=23.4 H=45.1 P=1023 C=23
```

---

### ⚙️ Setup

```bash
pip install pyserial
```

Edit configuration:

```python
SERIAL_PORT = "/dev/ttyACM0"   # Linux
# or COM5 on Windows
```

Run:

```bash
python3 serial_console.py
```

Exit:

```
Ctrl + C
```

---

## 🌐 esp32_logger.sh

A **background HTTP logger** for ESP32 sensor data.

This script:

* periodically queries the ESP32 `/status` endpoint
* parses JSON sensor data
* logs values to a file
* runs as a **systemd service**

---

### 🔧 Features

* Fully automatic startup (systemd)
* Robust error handling
* JSON parsing via `jq`
* Timestamped logs
* Designed for **24/7 operation**

---

### 📡 ESP32 API

Endpoint:

```
GET /status
```

Example response:

```json
{
  "temp": 23.4,
  "hum": 45.1,
  "photo": 1023,
  "clapAgo": 2.4
}
```

---

### 🗂 Logged Format

```
YYYY-MM-DD HH:MM:SS T=23.4 H=45.1 P=1023 C=2.4
```

---

### ⚙️ Setup & Installation

Install dependencies:

```bash
sudo apt install curl jq
```

Configure ESP32 IP:

```bash
ESP32_HOST="192.168.0.103"
```

Run installer:

```bash
chmod +x esp32_logger.sh
./esp32_logger.sh
```

Service management:

```bash
systemctl status esp32-logger
systemctl restart esp32-logger
journalctl -u esp32-logger
```

---

## 🔁 Data Flow Summary

```
ESP32 --HTTP--> Linux (logger)
Arduino <--I2C--> STM32 <--UART--> Linux (console)
```

Linux **never talks directly to Arduino** — all control goes through STM32.

---

## 💡 Design Highlights

* ✅ Clear separation of responsibilities
* ✅ Text-based protocols for transparency & debugging
* ✅ systemd integration (real Linux service, not a script loop)
* ✅ Works without GUI (headless-ready)
* ✅ Easy to extend (MQTT, DB, cloud upload)

---

## 🧪 Possible Improvements

* Export logs to CSV / JSON
* Push data to InfluxDB / Grafana
* Add watchdog for ESP32 availability
* Implement command history & autocomplete
* Encrypt STM32 ↔ Linux communication

---

> Built as part of **Embedded-ControlHub** — a multi-MCU embedded system focused on clean architecture, determinism, and real-world Linux integration. ⚙️🐧
