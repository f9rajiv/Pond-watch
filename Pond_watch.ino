#define BLYNK_TEMPLATE_ID "TMPL4VUO8vNZB"
#define BLYNK_TEMPLATE_NAME "Water monitoring"
#define BLYNK_AUTH_TOKEN    ENV_BLYNK_AUTH_TOKEN

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Arduino.h>
#include <ArduinoEnv.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DHT.h>
#include <ESP32Servo.h>

#include <EEPROM.h>
#include "DFRobot_ESP_PH.h"
#include <GardenSpine.h>

/************************************************************
 * GARDEN SPINE
 ************************************************************/
GardenSpine spine;

const unsigned long GARDENSPINE_INTERVAL = 5UL * 60UL * 1000UL; // 5 minutes
unsigned long lastGardenSpinePublish = 0;

/************************************************************
 * WIFI
 ************************************************************/
char ssid[] = "panoulu";
char pass[] = "";

/************************************************************
 * PIN DEFINITIONS
 ************************************************************/
#define PH_PIN          35
#define OLED_SDA        21
#define OLED_SCL        22
#define DHT_PIN         32
#define DHT_TYPE        DHT11
#define TRIG_PIN        26
#define ECHO_PIN        27
#define SERVO_PIN       13

/************************************************************
 * OLED CONFIGURATION
 ************************************************************/
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET       -1
#define OLED_ADDRESS     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/************************************************************
 * SENSOR & ACTUATOR OBJECTS
 ************************************************************/
DHT dht(DHT_PIN, DHT_TYPE);
Servo myServo;
DFRobot_ESP_PH ph;
BlynkTimer timer;

/************************************************************
 * SENSOR & SYSTEM VARIABLES
 ************************************************************/
float pHValue       = 7.0;
float pHVoltage     = 0.0;
float temperature   = 25.0;
float humidity      = 50.0;
float distanceCM    = 0.0;
float waterLevel    = 0.0; 

bool phAbnormal     = false;

const float PH_LOW_LIMIT  = 6.0;
const float PH_HIGH_LIMIT = 8.5;

/************************************************************
 * NON-BLOCKING SERVO SWEEP STATE MACHINE
 ************************************************************/
int currentServoAngle = 0;
int servoDirection    = 1;  
unsigned long lastServoStep = 0;
unsigned long pauseStartTime = 0;
bool isServoPausing = false;

const unsigned long STEP_INTERVAL = 33; 
const unsigned long PAUSE_INTERVAL = 500; 

/************************************************************
 * ALERT MANAGEMENT & TIMERS
 ************************************************************/
unsigned long lastPHAlert = 0;
const unsigned long PH_ALERT_INTERVAL = 60000UL;

const unsigned long PH_INTERVAL       = 500;
const unsigned long DHT_INTERVAL      = 2000;
const unsigned long DIST_INTERVAL     = 200;
const unsigned long OLED_INTERVAL     = 500;
const unsigned long BLYNK_INTERVAL    = 1000;

/************************************************************
 * FUNCTION DECLARATIONS
 ************************************************************/
void readPHSensor();
void readDHTSensor();
void readUltrasonic();
void updateOLED();
void updateBlynk();
void processPHAlert();
void updateServoSweep();
float readUltrasonicDistance();
void publishGardenSpine();

/************************************************************
 * SETUP
 ************************************************************/
void setup()
{
  Serial.begin(115200);
  delay(100);

  pinMode(PH_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  analogReadResolution(12);
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED init FAILED!");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Water Monitoring");
  display.println("Initializing...");
  display.display();

  dht.begin();

  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0);

  EEPROM.begin(32);
  ph.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Timers
  timer.setInterval(PH_INTERVAL, readPHSensor);
  timer.setInterval(DHT_INTERVAL, readDHTSensor);
  timer.setInterval(DIST_INTERVAL, readUltrasonic);
  timer.setInterval(OLED_INTERVAL, updateOLED);
  timer.setInterval(BLYNK_INTERVAL, updateBlynk);

  readDHTSensor();
  readUltrasonic();
  readPHSensor();
  updateOLED();
}

/************************************************************
 * LOOP
 ************************************************************/
void loop()
{
  Blynk.run();
  timer.run();

  updateServoSweep();

  ph.calibration(analogRead(PH_PIN), temperature);
  
  publishGardenSpine();

}

/************************************************************
 * SERVO SWEEP LOGIC (CONDITION: POLLUTED & LEVEL TRIGGER)
 ************************************************************/
void updateServoSweep()
{
  // Điều kiện quét: pH bất thường và có giá trị level hợp lệ (> 0)
  bool triggerCondition = (phAbnormal && waterLevel > 0.0);

  if (triggerCondition)
  {
    digitalWrite(RED_LED_PIN, HIGH);

    unsigned long currentMillis = millis();

    if (isServoPausing) {
      if (currentMillis - pauseStartTime >= PAUSE_INTERVAL) {
        isServoPausing = false;
        lastServoStep = currentMillis;
      }
      return;
    }

    if (currentMillis - lastServoStep >= STEP_INTERVAL) {
      currentServoAngle += servoDirection;
      myServo.write(currentServoAngle);
      lastServoStep = currentMillis;

      if (currentServoAngle >= 90) {
        currentServoAngle = 90;
        servoDirection = -1;
        isServoPausing = true;
        pauseStartTime = currentMillis;
      } 
      else if (currentServoAngle <= 0) {
        currentServoAngle = 0;
        servoDirection = 1;
        isServoPausing = true;
        pauseStartTime = currentMillis;
      }
    }
  }
  else
  {
    if (currentServoAngle != 0) {
      currentServoAngle = 0;
      myServo.write(0);
    }
    servoDirection = 1;
    isServoPausing = false;
    digitalWrite(RED_LED_PIN, phAbnormal ? HIGH : LOW);
  }
}

/************************************************************
 * READ pH SENSOR
 ************************************************************/
void readPHSensor()
{
  int rawADC = analogRead(PH_PIN);
  pHVoltage = ((float)rawADC / 4095.0) * 3300.0;
  pHValue = ph.readPH(pHVoltage, temperature);

  if (isnan(pHValue)) return;

  if (pHValue < PH_LOW_LIMIT || pHValue > PH_HIGH_LIMIT) {
    phAbnormal = true;
  } else {
    phAbnormal = false;
  }

  processPHAlert();
}

/************************************************************
 * DHT11
 ************************************************************/
void readDHTSensor()
{
  float newHum = dht.readHumidity();
  float newTemp = dht.readTemperature();

  if (!isnan(newHum) && !isnan(newTemp)) {
    humidity = newHum;
    temperature = newTemp;
  }
}

/************************************************************
 * HC-SR04 & LEVEL CALCULATION
 ************************************************************/
void readUltrasonic()
{
  distanceCM = readUltrasonicDistance();

  // Kiểm tra tránh chia cho 0 và cảm biến lỗi
  if (distanceCM > 0.0) {
    waterLevel = (distanceCM - 4.0) / distanceCM;
  } else {
    waterLevel = -1.0;
  }
}

float readUltrasonicDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return -1.0;

  return (duration * 0.0343 / 2.0);
}

/************************************************************
 * pH ALERT
 ************************************************************/
void processPHAlert()
{
  if (!phAbnormal) return;

  unsigned long currentMillis = millis();
  if (currentMillis - lastPHAlert >= PH_ALERT_INTERVAL) {
    Blynk.logEvent("ph_alert", "Abnormal water pH detected!");
    lastPHAlert = currentMillis;
  }
}

/************************************************************
 * OLED DISPLAY
 ************************************************************/
void updateOLED()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WATER MONITOR");
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  display.setCursor(0, 13);
  display.print("pH   : ");
  display.print(pHValue, 2);

  display.setCursor(0, 24);
  display.print("Temp : ");
  display.print(temperature, 1);
  display.println(" C");

  display.setCursor(0, 35);
  display.print("Hum  : ");
  display.print(humidity, 0);
  display.println(" %");

  display.setCursor(0, 46);
  display.print("Level: ");
  if (waterLevel < 0) {
    display.println("--");
  } else {
    display.print(waterLevel, 2);
  }

  display.setCursor(0, 56);
  if (phAbnormal && waterLevel > 0.0) {
    display.print("STATUS: SWEEP ACTIVE");
  } else if (phAbnormal) {
    display.print("STATUS: pH ALERT");
  } else {
    display.print("STATUS: NORMAL");
  }

  display.display();
}

/************************************************************
 * BLYNK DATA
 ************************************************************/
void updateBlynk()
{
  Blynk.virtualWrite(V0, pHValue);
  Blynk.virtualWrite(V1, pHVoltage);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, humidity);
  if (waterLevel >= 0) {
    Blynk.virtualWrite(V4, waterLevel); 
  }
}

/************************************************************
 * GARDEN SPINE PUBLISH
 ************************************************************/
 void publishGardenSpine()
 {
   unsigned long currentMillis = millis();
 
   if (currentMillis - lastGardenSpinePublish >= GARDENSPINE_INTERVAL)
   {
     lastGardenSpinePublish = currentMillis;
 
     // Water level as percentage
     float waterLevelPercent = waterLevel * 100.0;
 
     // Keep percentage within 0-100
     if (waterLevelPercent < 0.0) {
       waterLevelPercent = 0.0;
     }
 
     if (waterLevelPercent > 100.0) {
       waterLevelPercent = 100.0;
     }
 
     // Publish water level
     spine.publish(
       "waterlevel",
       waterLevelPercent,
       "percent"
     );
 
     // Status enum
     int status;
 
     if (phAbnormal && waterLevel > 0.0)
     {
       status = 2;   // SWEEP ACTIVE
     }
     else if (phAbnormal)
     {
       status = 1;   // pH ALERT
     }
     else
     {
       status = 0;   // NORMAL
     }
 
     spine.publish(
       "status",
       status,
       "enum"
     );
 
     Serial.println("GardenSpine published:");
     Serial.print("Water Level: ");
     Serial.print(waterLevelPercent);
     Serial.println(" %");
 
     Serial.print("Status: ");
     Serial.println(status);
   }
 }