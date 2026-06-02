# Testing and Validation

This document defines the testing procedures, validation criteria, and results for the Autonomous Industrial Line-Following Robot. All tests are designed to verify subsystem functionality before full system integration.

---

## Test Environment

| Parameter | Specification |
|-----------|--------------|
| Power Source | 7.4V LiPo battery (2S) or 9V alkaline battery |
| Test Surface | White surface with 25mm black electrical tape line |
| Ambient Light | Indoor, fluorescent lighting (typical lab conditions) |
| Measurement Tools | Digital multimeter, oscilloscope (if available) |
| Test Firmware | Arduino IDE with serial monitor for debug output |

---

## Test Plan Overview

| Test ID | Test Name | Priority | Prerequisites |
|---------|-----------|----------|---------------|
| T-01 | Power Supply Verification | High | None |
| T-02 | Arduino Nano Functional Check | High | T-01 |
| T-03 | Sensor Array Calibration | High | T-02 |
| T-04 | Motor Driver Validation | High | T-02 |
| T-05 | Individual Motor Test | High | T-04 |
| T-06 | PD Control Tuning | Medium | T-03, T-05 |
| T-07 | Straight-Line Tracking | Medium | T-06 |
| T-08 | Curve Navigation | Medium | T-07 |
| T-09 | Intersection Detection | Low | T-08 |
| T-10 | Endurance Testing | Low | T-08 |

---

## T-01: Power Supply Verification

**Objective**: Verify that the LM7805 voltage regulator provides a stable 5V output under varying load conditions.

**Equipment**: Digital multimeter

**Procedure**:

1. **No-Load Test**
   - Connect battery to J1 (SSQ-102-03-F-S)
   - Measure voltage at Arduino Nano 5V pin (no Nano installed)
   - Record: Expected ≈ 5.0V ± 0.1V

2. **Light Load Test**
   - Install Arduino Nano
   - Measure voltage at 5V rail
   - Record: Expected ≈ 4.9V–5.1V

3. **Full Load Test**
   - Install all modules (Nano, motor driver, sensors connected)
   - Run firmware with motors active
   - Measure voltage at 5V rail during motor operation
   - Record: Expected ≈ 4.7V–5.1V (allowing for regulator droop)

4. **Ripple Measurement** (Oscilloscope, if available)
   - Measure AC component on 5V rail during motor PWM operation
   - Record: Expected < 100mV peak-to-peak

**Pass Criteria**:
| Condition | Minimum | Maximum |
|-----------|---------|---------|
| No-load 5V | 4.90V | 5.10V |
| Full-load 5V | 4.70V | 5.10V |
| Ripple | — | 100mV p-p |

---

## T-02: Arduino Nano Functional Check

**Objective**: Verify Arduino Nano is operational and can execute basic I/O operations.

**Procedure**:

1. **Upload Test Sketch**
   ```cpp
   void setup() {
     Serial.begin(9600);
     pinMode(LED_BUILTIN, OUTPUT);
   }

   void loop() {
     digitalWrite(LED_BUILTIN, HIGH);
     Serial.println("Hello from Nano");
     delay(1000);
     digitalWrite(LED_BUILTIN, LOW);
     delay(1000);
   }
   ```

2. **Verify**
   - LED on Nano blinks at 1Hz
   - Serial monitor displays "Hello from Nano" every second
   - No communication errors

3. **ADC Verification**
   - Upload analog read sketch
   - Read A0–A4 with no sensor connected
   - Record: Expected ≈ 0–10 mV (floating, should read low)

---

## T-03: Sensor Array Calibration

**Objective**: Determine sensor threshold values for reliable line detection and calibrate weighted position calculation.

**Equipment**: Test track with black line on white surface

**Procedure**:

1. **Individual Sensor Test**
   - Upload sensor read sketch that outputs all 5 analog values via serial
   - Place sensor array over white surface (no line)
   - Record readings for each sensor (S1–S5)
   - Place sensor array over black line (centered)
   - Record readings for each sensor

   **Expected Results** (approximate):

   | Surface | Sensor Reading (ADC) |
   |---------|---------------------|
   | White surface | 600–900 |
   | Black line | 100–300 |

2. **Threshold Determination**
   - Calculate midpoint between white and black readings
   - Threshold = (White_Reading + Black_Reading) / 2
   - Example: If white = 800, black = 200, threshold = 500

3. **Position Calibration**
   - Place line under each sensor position
   - Verify weighted position calculation outputs expected values
   - Position values should range from 1.0 (far left) to 5.0 (far right)

4. **Noise Assessment**
   - Record 100 consecutive readings with line centered under S3
   - Calculate standard deviation
   - Acceptable: σ < 30 ADC counts

**Calibration Data Template**:

| Sensor | White Surface | Black Line | Threshold |
|--------|---------------|------------|-----------|
| S1 (A0) | ___ mV | ___ mV | ___ mV |
| S2 (A1) | ___ mV | ___ mV | ___ mV |
| S3 (A2) | ___ mV | ___ mV | ___ mV |
| S4 (A3) | ___ mV | ___ mV | ___ mV |
| S5 (A4) | ___ mV | ___ mV | ___ mV |

---

## T-04: Motor Driver Validation

**Objective**: Verify TB6612FNG motor driver responds correctly to control signals.

**Procedure**:

1. **Logic Supply Test**
   - Measure voltage at TB6612FNG VCC pin
   - Record: Expected ≈ 5.0V

2. **Motor Supply Test**
   - Measure voltage at TB6612FNG VM pin
   - Record: Expected ≈ Battery voltage (7.4V–9V)

3. **Direction Control Test**
   - Manually set AIN1=HIGH, AIN2=LOW via Arduino
   - Verify left motor spins forward
   - Set AIN1=LOW, AIN2=HIGH
   - Verify left motor spins reverse
   - Repeat for BIN1/BIN2 (right motor)

4. **PWM Speed Control Test**
   - Set motor to forward direction
   - Apply PWM duty cycles: 25%, 50%, 75%, 100%
   - Verify proportional speed response
   - Listen for smooth operation (no stuttering)

5. **Brake Test**
   - Set AIN1=HIGH, AIN2=HIGH
   - Verify motor stops abruptly (brake mode)
   - Set AIN1=LOW, AIN2=LOW
   - Verify motor coasts to stop (coast mode)

---

## T-05: Individual Motor Test

**Objective**: Verify both motors operate correctly when connected through the PCB.

**Procedure**:

1. **Left Motor Test**
   - Connect left motor to st1 connector
   - Run test firmware: full speed forward for 2 seconds, pause, full speed reverse for 2 seconds
   - Verify smooth operation in both directions
   - Measure motor current (if possible): Expected ~100–200mA no-load

2. **Right Motor Test**
   - Repeat procedure for right motor on st2

3. **Differential Test**
   - Run both motors forward at equal PWM
   - Verify robot moves straight (place on flat surface briefly)
   - Run left motor only, verify robot pivots left
   - Run right motor only, verify robot pivots right

---

## T-06: PD Control Tuning

**Objective**: Optimize Kp and Kd gain parameters for stable line following.

**Initial Parameters**:
```
Kp = 25.0    (proportional gain)
Kd = 10.0    (derivative gain)
Base_Speed = 150  (PWM value, 0–255)
```

**Tuning Procedure**:

1. **Proportional Tuning (Kp)**
   - Set Kd = 0
   - Start with Kp = 10
   - Place robot on line, observe behavior
   - Increase Kp by increments of 5 until robot oscillates around the line
   - Record Kp_oscillation
   - Set Kp = Kp_oscillation × 0.6

2. **Derivative Tuning (Kd)**
   - With Kp set, start with Kd = 0
   - Increase Kd by increments of 2
   - Observe: Kd should reduce oscillation without causing sluggish response
   - Stop when oscillation is acceptable but response remains fast

3. **Speed Tuning**
   - With Kp and Kd set, adjust Base_Speed
   - Start low (100) and increase gradually
   - Find maximum speed where robot reliably follows the line

**Expected Outcome**:
- Robot follows straight line with minimal oscillation
- Robot navigates gentle curves (< 90°) without losing the line
- No wheel stall or erratic behavior

---

## T-07: Straight-Line Tracking Test

**Objective**: Validate robot can autonomously follow a straight line.

**Test Track**: 2-meter straight black line on white surface

**Procedure**:

1. Place robot on track with line centered under sensor array
2. Start firmware
3. Observe robot following the line
4. Record:
   - Does robot stay on line for full 2 meters? (Y/N)
   - Number of corrections (lateral movements)
   - Average deviation from center (visual estimate)

**Pass Criteria**:
- Robot completes 2-meter straight line without leaving the track
- Smooth, minimal oscillation
- No manual intervention required

---

## T-08: Curve Navigation Test

**Objective**: Validate robot can navigate curves of varying radii.

**Test Track**: Track with curves of decreasing radius (large → medium → tight)

**Procedure**:

1. Run robot on track with 500mm radius curve
2. Run robot on track with 200mm radius curve
3. Run robot on track with 100mm radius curve (if available)
4. Record success/failure at each radius

**Pass Criteria**:
- Successfully navigates 200mm radius curves
- May lose line at 100mm radius (acceptable for basic implementation)

---

## T-09: Intersection Detection Test (Future Enhancement)

**Objective**: Test ability to detect and respond to track intersections.

> **Note**: This test requires firmware modifications to implement intersection detection logic. Currently a placeholder for future development.

---

## T-10: Endurance Test

**Objective**: Verify sustained operation over extended period.

**Procedure**:

1. Fully charge battery
2. Start robot on a closed-loop track (if available)
3. Record runtime until battery depletion or system failure
4. Monitor for:
   - Thermal issues (motor driver, regulator)
   - Cumulative drift in line following
   - Any intermittent failures

**Expected Runtime**: 15–30 minutes depending on battery capacity and motor load.

---

## Common Issues and Troubleshooting

| Symptom | Probable Cause | Solution |
|---------|---------------|----------|
| Robot veers strongly left/right | Sensor calibration incorrect | Recalibrate sensor thresholds |
| Robot oscillates wildly | Kp too high | Reduce Kp gain |
| Robot responds slowly to curves | Kp too low or Kd too high | Increase Kp or reduce Kd |
| One motor doesn't spin | Loose connector or dead driver channel | Check st1/st2 connections |
| Robot runs in circles | Motor wiring reversed | Swap motor terminal connections |
| 5V rail drops below 4.5V | Battery depleted or regulator fault | Replace battery or check regulator |
| Erratic sensor readings | Electrical noise, loose sensor | Check sensor mounting height, add filtering |
| Robot stops unexpectedly | Motor stall or brown-out | Check battery voltage, reduce speed |

---

## Test Result Documentation

All test results should be recorded in the following format:

```
Test ID:    T-XX
Date:       YYYY-MM-DD
Tester:     [Name]
Result:     PASS / FAIL / PARTIAL
Notes:      [Observations]
Data:       [Measured values]
```

---

## Safety Notes

- Disconnect battery before making wiring changes
- Do not touch motor terminals during operation
- Ensure robot has clear space around the test area
- Keep fingers away from moving wheels
- Monitor component temperatures during initial testing
