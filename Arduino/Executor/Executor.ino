/**
 * @file Executor.ino
 * @brief I2C-controlled actuator board with servo, stepper motor, relay, and button.
 *
 * Acts as an I2C slave device that receives text-based commands
 * to control a servo motor, stepper motor, and relay.
 * Also reports button press events back to the I2C master.
 */

#include <Wire.h>
#include <Servo.h>
#include <Stepper.h>

// -----------------------------------------------------------------------------
// I2C Configuration
// -----------------------------------------------------------------------------

/** @brief I2C slave address */
#define I2C_ADDR 0x08

/** @brief I2C buffer size */
#define I2C_BUF  32

volatile bool cmdReady = false;
char i2cCmd[I2C_BUF];
char i2cResp[I2C_BUF] = "READY";

// -----------------------------------------------------------------------------
// Pin Configuration
// -----------------------------------------------------------------------------

#define SERVO_PIN   5
#define RELAY_PIN   7
#define BUTTON_PIN  6

#define IN1 8
#define IN2 9
#define IN3 10
#define IN4 11

// -----------------------------------------------------------------------------
// Devices & State
// -----------------------------------------------------------------------------

/** @brief Stepper motor steps per full revolution */
#define STEPS_PER_REV 2048

Stepper stepper(STEPS_PER_REV, IN1, IN3, IN2, IN4);
Servo servo;

int  servoPos   = 90;
int  stepperPos = 0;
bool relayState = false;
bool btnEvent   = false;

// -----------------------------------------------------------------------------
// Function Declarations
// -----------------------------------------------------------------------------

void onI2CReceive(int len);
void onI2CRequest();
void handleCommand();
void handleButton();

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

/**
 * @brief Initializes I2C, GPIO, and connected devices.
 */
void setup() {
  Wire.begin(I2C_ADDR);
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);

  servo.attach(SERVO_PIN);
  servo.write(servoPos);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  stepper.setSpeed(10);
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------

/**
 * @brief Handles incoming I2C commands and button events.
 */
void loop() {
  if (cmdReady) {
    cmdReady = false;
    handleCommand();
  }

  handleButton();
}

// -----------------------------------------------------------------------------
// I2C Callbacks
// -----------------------------------------------------------------------------

/**
 * @brief Receives command data from I2C master.
 */
void onI2CReceive(int len) {
  int i = 0;
  while (Wire.available() && i < I2C_BUF - 1) {
    i2cCmd[i++] = Wire.read();
  }
  i2cCmd[i] = '\0';
  cmdReady = true;
}

/**
 * @brief Sends response data to I2C master.
 *
 * If a button press event occurred, it is appended to the response.
 */
void onI2CRequest() {
  char tmpResp[I2C_BUF];
  strncpy(tmpResp, i2cResp, I2C_BUF - 1);
  tmpResp[I2C_BUF - 1] = '\0';

  // Append button press event information if detected
  if (btnEvent) {
    strncat(tmpResp, " +BTN PRESSED",
            I2C_BUF - strlen(tmpResp) - 1);
    btnEvent = false; // Clear event flag
  }

  Wire.write(tmpResp, strlen(tmpResp) + 1);
}

// -----------------------------------------------------------------------------
// Command Handler
// -----------------------------------------------------------------------------

/**
 * @brief Parses and executes received I2C commands.
 */
void handleCommand() {
  if (strncmp(i2cCmd, "SERVO SET", 9) == 0) {
    servoPos = constrain(atoi(i2cCmd + 10), 0, 180);
    servo.write(servoPos);
    strcpy(i2cResp, "OK SERVO");
  }
  else if (strncmp(i2cCmd, "STEPPER MOVE", 12) == 0) {
    strcpy(i2cResp, "STEPPER BUSY");
    int steps = atoi(i2cCmd + 13);
    stepper.step(steps);
    stepperPos += steps;
    strcpy(i2cResp, "OK STEPPER");
  }
  else if (strcmp(i2cCmd, "RELAY ON") == 0) {
    digitalWrite(RELAY_PIN, HIGH);
    relayState = true;
    strcpy(i2cResp, "OK RELAY ON");
  }
  else if (strcmp(i2cCmd, "RELAY OFF") == 0) {
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
    strcpy(i2cResp, "OK RELAY OFF");
  }
  else if (strcmp(i2cCmd, "STATUS") == 0) {
    snprintf(i2cResp, I2C_BUF,
             "SERVO=%d RELAY=%s STEPPER=%d",
             servoPos,
             relayState ? "ON" : "OFF",
             stepperPos);
  }
  else {
    strcpy(i2cResp, "ERR CMD");
  }
}

// -----------------------------------------------------------------------------
// Button Handling
// -----------------------------------------------------------------------------

/**
 * @brief Detects button press events (falling edge).
 */
void handleButton() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW) {
    btnEvent = true;
  }

  lastState = currentState;
}
