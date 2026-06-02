/*
 * Autonomous Industrial Line-Following Robot
 * Main Firmware
 *
 * Hardware:
 *   - Arduino Nano (ATmega328P)
 *   - TB6612FNG Dual H-Bridge Motor Driver
 *   - 5x TCRT5000 IR Reflective Sensors
 *   - 2x N20 DC Gear Motors
 *
 * Pin Assignments:
 *   A0-A4  : IR Sensor Array (analog inputs)
 *   D2     : AIN1 (Motor A direction)
 *   D3     : AIN2 (Motor A direction)
 *   D4     : BIN1 (Motor B direction)
 *   D5     : PWMA (Motor A speed - PWM)
 *   D6     : PWMB (Motor B speed - PWM)
 *   D7     : BIN2 (Motor B direction)
 */

// ==================== Pin Definitions ====================
#define SENSOR_1    A0
#define SENSOR_2    A1
#define SENSOR_3    A2
#define SENSOR_4    A3
#define SENSOR_5    A4

#define AIN1        2
#define AIN2        3
#define BIN1        4
#define BIN2        7
#define PWMA        5
#define PWMB        6

// ==================== Control Parameters ====================
#define BASE_SPEED      150     // Base motor speed (0-255)
#define MAX_SPEED       255     // Maximum motor speed
#define MIN_SPEED       80      // Minimum speed for motor stall prevention

// PD Gains (tune these for your robot)
float Kp = 25.0;                // Proportional gain
float Kd = 10.0;                // Derivative gain

// ==================== Sensor Configuration ====================
#define NUM_SENSORS     5
#define THRESHOLD       500     // ADC threshold (adjust per calibration)
#define CENTER_POSITION 3.0     // Ideal line position (center of array)

// Sensor weights for position calculation
const float sensorWeights[NUM_SENSORS] = {1.0, 2.0, 3.0, 4.0, 5.0};

// ==================== State Variables ====================
float previousError = 0.0;
unsigned long lastLoopTime = 0;
const unsigned long LOOP_INTERVAL = 10; // milliseconds

// ==================== Motor Control Functions ====================

/*
 * Sets the direction and speed of Motor A (left motor).
 * direction: 1 = forward, -1 = reverse, 0 = brake
 * speed: 0-255 PWM value
 */
void setMotorA(int direction, int speed) {
    speed = constrain(speed, 0, MAX_SPEED);

    if (direction == 1) {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
    } else if (direction == -1) {
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);
    } else {
        // Brake
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, HIGH);
        speed = 0;
    }

    analogWrite(PWMA, speed);
}

/*
 * Sets the direction and speed of Motor B (right motor).
 * direction: 1 = forward, -1 = reverse, 0 = brake
 * speed: 0-255 PWM value
 */
void setMotorB(int direction, int speed) {
    speed = constrain(speed, 0, MAX_SPEED);

    if (direction == 1) {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, LOW);
    } else if (direction == -1) {
        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, HIGH);
    } else {
        // Brake
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, HIGH);
        speed = 0;
    }

    analogWrite(PWMB, speed);
}

/*
 * Stops both motors immediately.
 */
void stopMotors() {
    setMotorA(0, 0);
    setMotorB(0, 0);
}

// ==================== Sensor Functions ====================

/*
 * Reads all 5 sensor values and calculates the weighted line position.
 * Returns: Position value from 1.0 (far left) to 5.0 (far right)
 *          Returns 0.0 if no line is detected.
 */
float readLinePosition() {
    int sensorValues[NUM_SENSORS];
    float weightedSum = 0.0;
    float totalWeight = 0.0;

    // Read all sensors
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensorValues[i] = analogRead(SENSOR_1 + i);
    }

    // Calculate weighted position
    for (int i = 0; i < NUM_SENSORS; i++) {
        // Only include sensors that detect the line (below threshold)
        if (sensorValues[i] < THRESHOLD) {
            weightedSum += sensorWeights[i] * (float)sensorValues[i];
            totalWeight += (float)sensorValues[i];
        }
    }

    // Avoid division by zero
    if (totalWeight == 0.0) {
        return 0.0; // No line detected
    }

    return weightedSum / totalWeight;
}

/*
 * Returns true if the line is detected by any sensor.
 */
bool isLineDetected() {
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (analogRead(SENSOR_1 + i) < THRESHOLD) {
            return true;
        }
    }
    return false;
}

// ==================== Setup ====================

void setup() {
    // Initialize serial for debugging
    Serial.begin(9600);
    Serial.println("Line Following Robot Initializing...");

    // Motor control pins
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(PWMB, OUTPUT);

    // Sensor pins (analog inputs, no pinMode needed)

    // Initialize motors to stopped state
    stopMotors();

    // Small delay for system stabilization
    delay(500);

    Serial.println("Initialization complete. Starting line following...");
}

// ==================== Main Loop ====================

void loop() {
    unsigned long currentTime = millis();

    // Fixed interval control loop
    if (currentTime - lastLoopTime >= LOOP_INTERVAL) {
        lastLoopTime = currentTime;

        // Read line position
        float position = readLinePosition();

        // Check if line is detected
        if (position == 0.0) {
            // Line lost - stop or implement recovery behavior
            stopMotors();
            Serial.println("Line lost!");
            return;
        }

        // Calculate error (deviation from center)
        float error = position - CENTER_POSITION;

        // PD Control
        float derivative = error - previousError;
        float correction = (Kp * error) + (Kd * derivative);

        // Save error for next iteration
        previousError = error;

        // Calculate motor speeds
        int leftSpeed = BASE_SPEED + (int)correction;
        int rightSpeed = BASE_SPEED - (int)correction;

        // Constrain to valid range
        leftSpeed = constrain(leftSpeed, MIN_SPEED, MAX_SPEED);
        rightSpeed = constrain(rightSpeed, MIN_SPEED, MAX_SPEED);

        // Set motor directions and speeds
        setMotorA(1, leftSpeed);    // Left motor forward
        setMotorB(1, rightSpeed);   // Right motor forward

        // Debug output (uncomment for tuning)
        /*
        Serial.print("Pos: ");
        Serial.print(position, 2);
        Serial.print(" Err: ");
        Serial.print(error, 2);
        Serial.print(" L: ");
        Serial.print(leftSpeed);
        Serial.print(" R: ");
        Serial.println(rightSpeed);
        */
    }
}
