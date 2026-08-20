# Pond Watch

## Botanical Garden Pond Monitoring System

**Group:** AQUA
**Team:** Sam, Nhat, Rajiv, Darren
**Project period:** 10 August 2026 to 21 August 2026
**Deployment:** University of Oulu Botanical Garden

## 1. Project

Pond Watch is an ESP32 based system for monitoring the pond at the University of Oulu Botanical Garden.

The system measures:

* Pond water pH using the SEN0161
* Ambient temperature and humidity using the DHT11
* Water level using the HC-SR04
* Sensor values and system status on a 0.96 inch I2C OLED
* Remote readings through Wi-Fi and Blynk

An MG669R TowerPro servo moves the pH probe using a single DOF articulated arm. The probe is lowered into the pond for measurement and then raised again.

The design changed during the first week after testing the available components and the first mechanical version.

## 2. Hardware

| Component                  | Purpose                          |
| -------------------------- | -------------------------------- |
| ESP32                      | Main controller                  |
| SEN0161                    | Water pH measurement             |
| DHT11                      | Ambient temperature and humidity |
| HC-SR04                    | Water level estimation           |
| 0.96 inch I2C OLED         | Local display                    |
| MG669R Tower Pro           | Probe arm actuator               |
| 4 x AA batteries           | Servo power                      |
| 3D printed arm and housing | Probe deployment mechanism       |
| Wi-Fi                      | Wireless connection              |
| Blynk                      | Remote monitoring                |

A water temperature sensor and turbidity sensor were not available, so they were removed from the design. The DHT11 is therefore used for ambient temperature rather than water temperature.

## 3. Wiring

The electronics were assembled around the ESP32 using a breadboard during testing.

| Device       | ESP32 connection      | Notes                                         |
| ------------ | --------------------- | --------------------------------------------- |
| SEN0161      | ADC input             | Analog pH signal                              |
| DHT11        | Digital GPIO          | Temperature and humidity                      |
| HC-SR04      | Trigger and Echo GPIO | Echo requires suitable ESP32 voltage handling |
| OLED         | I2C SDA / SCL         | OLED address: `0x3C`                          |
| MG669R    | Servo control GPIO    | Separate supply used for servo                |
| Servo supply | 4 x AA, about 6 V     | Not powered directly from the ESP32           |

The exact GPIO numbers should be kept in the current ESP32 source code as the wiring reference. The important power decision was to keep the MG669R on its own approximately 6 V supply instead of connecting it directly to the 9 V battery supplied with the ESP32 kit.

## 4. Mechanical Design

The final mechanism uses a single articulated arm with direct servo drive.

The arm is approximately:

* Length: 220 mm
* Width: 18 mm
* Thickness: 10 mm
* Moving assembly: approximately 120 g

The first actuator tested was an SG90. It could move the unloaded mechanism, but its approximately 1.8 kg/cm stall torque was below the calculated static requirement.

For the loaded arm:

`τ = 0.22 x 0.12 x 9.81`

This gives approximately **0.259 N m**, or **2.64 kg/cm**.

The MG669R was selected because its rated torque is approximately **5.5 kg/cm at 4.8 V**. This gives about a 2.08 times torque margin over the calculated static requirement.

The MG669R is a continuous rotation servo, so the system does not command a normal target angle. Instead, movement is controlled using calibrated pulse duration and timing.

Current calibration gives approximately:

* Arm movement: 75 degrees
* Lowering time: 0.65 s
* Raising time: 0.61 s

## 5. Software

The first ESP32 program used sequential operations and `delay()` calls. This caused problems because the controller could not handle other tasks normally while the arm was moving.

The software was changed to use `millis()` based timing.

Different tasks now run at different intervals:

| Task  | Approximate interval |
| ----- | -------------------: |
| pH    |                  1 s |
| OLED  |                  1 s |
| DHT11 |                  2 s |
| Blynk |                  2 s |

The pH reading uses an average of 10 ADC samples instead of relying on a single reading. This reduced short term variation in the raw measurement.

The DHT11 and HC-SR04 also have basic error handling. Failed DHT11 readings and HC-SR04 timeouts are not treated as normal sensor values.

## 6. Dashboard

Blynk was added for remote monitoring.

The dashboard receives the sensor data from the ESP32 over Wi-Fi. During testing, the Blynk update interval was set to approximately 2 seconds to avoid sending unnecessary updates.

Dashboard evidence should be included here:

**Dashboard screenshot:**

The screenshot should show the live pH, temperature, humidity and water level values.

**Serial Monitor evidence:**

This should show the ESP32 reading the sensors and reporting the system status.

## 7. Testing Evidence

### pH sensor

The SEN0161 was connected directly to the ESP32 ADC and tested in a test liquid.

During approximately 30 seconds of testing, the displayed pH changed by approximately 0.2 pH units. Ten ADC readings were then averaged to reduce short term variation.

**Evidence:**


### Servo and arm

The SG90 was rejected after the torque calculation showed that its approximately 1.8 kg/cm rating was below the estimated 2.64 kg/cm static requirement.

The MG669R successfully moved the loaded mechanism.

**Evidence:**

### OLED

The 0.96 inch OLED was tested successfully. Its I2C address was found to be `0x3C`.

**Evidence:**

### 3D printed mechanism

The first print-in-place hinge used approximately 0.15 mm clearance. After printing, the moving surfaces fused and the hinge could not articulate.

The hinge was redesigned with approximately 0.4 mm clearance.

**Evidence:**

### Software timing

The original blocking `delay()` approach was replaced with `millis()` timing. This allows the ESP32 to continue handling sensor updates and Blynk communication while the servo mechanism is operating.

**Evidence:**

## 8. Current Limitations

There are several limitations in the current version.

1. **No water temperature measurement**
   The DHT11 measures ambient temperature. It does not measure the pond water temperature.

2. **No turbidity measurement**
   A suitable turbidity sensor was not available during development.

3. **Servo position is time based**
   The MG669R is a continuous rotation servo. The arm position is controlled through calibrated movement time rather than direct position feedback.

4. **Mechanical repeatability still needs testing**
   The first mounting structure moved by approximately 8 mm during upward movement. The redesigned structure needs repeated cycle testing to confirm that the arm returns to the same position.

5. **3D printing takes time**
   The revised assembly required approximately 4 hours and 20 minutes to print. This limited the number of physical design iterations.

6. **pH readings require calibration and stable conditions**
   The SEN0161 reading changes during operation, so the probe needs proper calibration and enough time in the water before relying on a measurement.

7. **The HC-SR04 gives an estimated water level**
   The sensor measures distance to the water surface. It does not directly measure water volume.

## 9. What Changed During Development

The original plan was to test the individual sensors and then combine the electronics and mechanical system.

The main changes were made after testing.

The SG90 was replaced by the MG669R because the SG90 did not provide enough torque for the calculated load. The original hinge was also redesigned because the 0.15 mm clearance was too small for the 3D printing process.

The software was changed from blocking `delay()` calls to `millis()` timing. This was needed so the ESP32 could continue handling the other system functions while the probe mechanism was moving.


## 10. Next Tests

The remaining work is mainly system testing.

* Complete the revised arm assembly.
* Test the 0.4 mm hinge clearance after printing.
* Run at least 10 complete raise and lower cycles.
* Check for binding and position drift.
* Check that the pH probe returns to approximately the same position.
* Test the complete pH measurement sequence.
* Confirm that Blynk communication continues while the arm is moving.
* Complete the OLED display.
* Test all sensors together.
* Begin Garden Spine communication.
* Record the final servo timing and repeated-cycle results.

## 11. Evidence Checklist

* [ ] ESP32 wiring photo
* [ ] SEN0161 pH test photo
* [ ] DHT11 test output
* [ ] HC-SR04 test output
* [ ] OLED display photo
* [ ] MG669R servo test photo
* [ ] 3D printed arm photo
* [ ] Revised hinge photo
* [ ] CAD screenshot
* [ ] ESP32 code screenshot
* [ ] Serial Monitor screenshot
* [ ] Blynk dashboard screenshot
* [ ] Full system test photo or video
* [ ] Repeated 10 cycle test results

## 12. Summary

Pond Watch now has the main sensing, display, communication and mechanical components working as separate parts and has started operating them together.

The biggest changes during Week 1 came from actual hardware testing. The SG90 was not strong enough for the calculated load, the first printed hinge had too little clearance, and blocking servo delays interfered with the ESP32 software. These issues were addressed by moving to the MG669R, increasing the hinge clearance to approximately 0.4 mm and changing the software to `millis()` timing.

The next step is to test the complete mechanism repeatedly and verify that the sensing, probe movement, OLED and Blynk dashboard all work together.
