/*
 * ============================================================
 *  Fire-Fighting Robot — ESP32 Firmware
 *  File:   firmware/esp32_safety_system.ino
 *  Logic:  Detect -> Move -> Spray
 *
 *  Pin map (source of truth: docs/hardware.md):
 *    Flame LEFT   GPIO 34 (input-only)
 *    Flame RIGHT  GPIO 35 (input-only)
 *    Flame CENTER GPIO 32
 *    IN1/IN2 (Left)  26/27   IN3/IN4 (Right) 14/13
 *    ENA (Left PWM)  25      ENB (Right PWM) 33  (LEDC)
 *    Water pump      23 (via 2N2222)
 *
 *  Docs: docs/wiring.md | docs/testing.md
 * ============================================================
 */

// ------------------- Pin Definitions -------------------
#define FLAME_LEFT_PIN    34   // input-only
#define FLAME_RIGHT_PIN   35   // input-only
#define FLAME_CENTER_PIN  32

#define MOTOR_L_IN1       26
#define MOTOR_L_IN2       27
#define MOTOR_R_IN3       14
#define MOTOR_R_IN4       13

#define_ENA         25   // Left  PWM
#define MOTOR_ENB         33   // Right PWM

#define PUMP_PIN          23

// ------------------- PWM Setup (LEDC) -------------------
#define LEDC_CH_L         0    // Left motor channel
#define LEDC_CH_R         1    // Right motor channel
#define LEDC_FREQ         1000 // 1 kHz PWM
#define LEDC_RES          8    // 8-bit -> duty 0..255

// ------------------- Tuning Constants -------------------
#define DRIVE_SPEED       180  // normal forward speed (0-255)
#define TURN_SPEED        160  // pivot speed
#define STOP_SPEED        0

#define PUMP_BURST_MS     2500 // spray duration per activation
#define FIRE_HOLD_MS      300  // fire must be seen this long before reacting (debounce)
#define COOLDOWN_MS       1000 // pause between pump bursts

// ------------------- State Machine -------------------
enum State { IDLE, SEEK_FIRE, APPROACH, SPRAY, COOLDOWN, FIRE_OUT };
State currentState = IDLE;

// ------------------- Timers -------------------
unsigned long fireFirstSeen = 0;   // debounce timer
unsigned long sprayStart    = 0;
unsigned long cooldownStart = 0;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  // --- Flame sensors ---
  pinMode(FLAME_LEFT_PIN,   INPUT);
  pinMode(FLAME_RIGHT_PIN,  INPUT);
  pinMode(FLAME_CENTER_PIN, INPUT);

  // --- Motor direction pins ---
  pinMode(MOTOR_L_IN1, OUTPUT);
  pinMode(MOTOR_L_IN2, OUTPUT);
  pinMode(MOTOR_R_IN3, OUTPUT);
  pinMode(MOTOR_R_IN4 OUTPUT);

  // --- PWM channels for ENA/ENB ---
  ledcSetup(LEDC_CH_L, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(MOTOR_ENA, LEDC_CH_L);
  ledcSetup(LEDC_CH_R, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(MOTOR_ENB, LEDC_CH_R);

  // --- Pump ---
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  stopMotors();
  pumpOff();

  Serial.println();
  Serial.println("========================================");
  Serial.println(" Fire-Fighting Robot v1.0");
  Serial.println(" Logic: Detect -> Move -> Spray");
  Serial.println("========================================");
  printSensors();
  Serial.println("Booting complete. Watching for fire...");
}

// ============================================================
//  MAIN LOOP — non-blocking state machine
// ============================================================
void loop() {
  // 1. READ SENSORS (active LOW: LOW = fire detected)
  bool fireLeft   = (digitalRead(FLAME_LEFT_PIN)   == LOW);
  bool fireRight  = (digitalRead(FLAME_RIGHT_PIN)  == LOW);
  bool fireCenter = (digitalRead(FLAME_CENTER_PIN) == LOW);

  // 2. PRINT STATUS (~2x per second for testing.md Stage 1/2)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    printSensors();
    Serial.print("State: "); Serial.println(stateName(currentState));
  }

  // 3. STATE MACHINE
  switch (currentState) {

    case IDLE:
      stopMotors();
      pumpOff();
      if (fireLeft || fireRight || fireCenter) {
        fireFirstSeen = millis();
        currentState = SEEK_FIRE;
        Serial.println(">>> FIRE DETECTED — engaging");
      }
      break;

    case SEEK_FIRE:
      if (!fireLeft && !fireRight && !fireCenter) {
        Serial.println(">>> Fire lost before approach. Stopping.");
        currentState = IDLE;
        break;
      }
      // Turn toward the fire until the center sensor locks on
      if (fireLeft && !fireCenter) {
        Serial.println("LEFT: FIRE   -> turning LEFT");
        turnLeft();
      } else if (fireRight && !fireCenter) {
        Serial.println("RIGHT: FIRE  -> turning RIGHT");
        turnRight();
      } else if (fireCenter) {
        Serial.println("CENTER FIRE -> driving FORWARD");
        currentState = APPROACH;
      }
      break;

    case APPROACH:
      if (fireCenter) {
        driveForward();
        // Small debounce: only spray if fire held steady
        if (millis() - fireFirstSeen > FIRE_HOLD_MS + 500) {
          stopMotors();
          sprayStart = millis();
          pumpOn();
          Serial.println(">>> Close range locked — PUMP ON");
          currentState = SPRAY;
        }
      } else if (fireLeft || fireRight) {
        // Fire drifted off-center while approaching — reaim
        currentState = SEEK_FIRE;
      } else {
        Serial.println(">>> Fire lost during approach. Stopping.");
        currentState = IDLE;
      }
      break;

    case SPRAY:
      if (millis() - sprayStart >= PUMP_BURST_MS) {
        pumpOff();
        Serial.println(">>> Pump burst done");
        cooldownStart = millis();
        currentState = COOLDOWN;
      }
      break;

    case COOLDOWN:
      stopMotors();
      if (millis() - cooldownStart >= COOLDOWN_MS) {
        if (fireLeft || fireRight || fireCenter) {
          Serial.println(">>> Fire still present — re-engaging");
          fireFirstSeen = millis();
          currentState = SEEK_FIRE;
        } else {
          Serial.println(">>> FIRE OUT. All clear. Stopping.");
          currentState = FIRE_OUT;
        }
      }
      break;

    case FIRE_OUT:
      stopMotors();
      pumpOff();
      // Robot stays parked. Reset EN button to run again,
      // or auto-rearm if desired: uncomment next lines:
      // currentState = IDLE;
      break;
  }
}

// ============================================================
//  MOTOR CONTROL
// ============================================================
void setLeft(int dir1, int dir2, int speed) {
  digitalWrite(MOTOR_L_IN1, dir1);
  digitalWrite(MOTOR_L_IN2, dir2);
  ledcWrite(LEDC_CH_L, speed);
}

void setRight(int dir3, int dir4, int speed) {
  digitalWrite(MOTOR_R_IN3, dir3);
  digitalWrite(MOTOR_R_IN4, dir4);
  ledcWrite(LEDC_CH_R, speed);
}

void driveForward() { setLeft(HIGH, LOW, DRIVE_SPEED); setRight(HIGH, LOW, DRIVE_SPEED); }
void turnLeft()     { setLeft(LOW, HIGH, TURN_SPEED);  setRight(HIGH, LOW, TURN_SPEED); }
void turnRight()    { setLeft(HIGH, LOW, TURN_SPEED);  setRight(LOW, HIGH, TURN_SPEED); }
void stopMotors()   { setLeft(LOW, LOW, STOP_SPEED);   setRight(LOW, LOW, STOP_SPEED); }

// ============================================================
//  PUMP CONTROL
// ============================================================
void pumpOn()  { digitalWrite(PUMP_PIN, HIGH); Serial.println("PUMP ON");  }
void pumpOff() { digitalWrite(PUMP_PIN, LOW);  Serial.println("PUMP OFF"); }

// ============================================================
//  DEBUG HELPERS
// ============================================================
void printSensors() {
  Serial.print("LEFT: ");
  Serial.print(digitalRead(FL_LEFT_PIN)   == LOW ? "FIRE  " : "clear ");
  Serial.print(" | RIGHT: ");
  Serial.print(digitalRead(FLAME_RIGHT_PIN)  == LOW ? "FIRE  " : "clear ");
  Serial.print(" | CENTER: ");
  Serial(digitalRead(FLAME_CENTER_PIN) == LOW ? "FIRE" : "clear");
}

const char* stateName(State s) {
  switch (s) {
    case IDLE:      return "IDLE";
    case SEEK_FIRE: return "SEEK_FIRE";
    case APPROACH:  return "APPROACH";
    case SPRAY:     return "SPRAY";
    case COOLDOWN:  return "COOLDOWN";
    case FIRE_OUT:  return "FIRE_OUT";
  }
  return "?";
}
