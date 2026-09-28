#define BLYNK_TEMPLATE_ID "TMPL3mJPIZgRA"
#define BLYNK_TEMPLATE_NAME "Smart Pet Feeder"
#define BLYNK_AUTH_TOKEN "MJDYJpEg2Zq7qZGjm0AsFi2Mw97XPsat"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <HX711.h>  

char ssid[] = "moto g85 5G_1168";
char pass[] = "6360007936";

#define SERVO_PIN 12
#define BUTTON_PIN 27

#define LED1 26
#define LED2 25
#define LED3 33
#define LED4 32
#define BUZZER 14

#define DT 4
#define SCK 5

HX711 scale;

Servo feederServo;
WidgetRTC rtc;
BlynkTimer timer;

bool feeding = false;
bool buttonPressed = false;

int feedHour = -1;
int feedMinute = -1;

bool scheduleTriggered = false;

String lastFeedTime = "Never";

float calibration_factor = 2280;
float empty_bowl_weight = 0;
float current_weight = 0;
float target_weight = 0;
float food_amount = 50;   // grams (can link to Blynk later)

void feedNow()
{
  if(feeding) return;

  feeding = true;

  Serial.println("Feeding started");

  Blynk.virtualWrite(V2,"Feeding in progress");

  digitalWrite(BUZZER,HIGH);

  target_weight = empty_bowl_weight + food_amount;

  while(true)
  {
    current_weight = scale.get_units(5);

    Serial.print("Weight: ");
    Serial.println(current_weight);

    if(current_weight >= target_weight)
    {
      break;
    }

    feederServo.write(60);

    digitalWrite(LED1,!digitalRead(LED1));
    digitalWrite(LED2,!digitalRead(LED2));
    digitalWrite(LED3,!digitalRead(LED3));
    digitalWrite(LED4,!digitalRead(LED4));

    Blynk.run();
    delay(200);

    feederServo.write(0);
    delay(200);
  }

  feederServo.write(0);
  digitalWrite(BUZZER,LOW);

  char buffer[20];
  sprintf(buffer,"%02d:%02d:%02d",hour(),minute(),second());

  lastFeedTime = String(buffer);

  Serial.print("Feeding completed at ");
  Serial.println(lastFeedTime);

  Blynk.virtualWrite(V2,"Last Feed: " + lastFeedTime);

  feeding = false;
}

BLYNK_WRITE(V0)
{
  if(param.asInt()==1)
  {
    Serial.println("Feed from phone");
    feedNow();
  }
}

BLYNK_WRITE(V3)
{
  TimeInputParam t(param);

  if(t.hasStartTime())
  {
    feedHour = t.getStartHour();
    feedMinute = t.getStartMinute();

    Serial.print("Schedule received: ");
    Serial.print(feedHour);
    Serial.print(":");
    Serial.println(feedMinute);
  }
}

void checkSchedule()
{
  if(feedHour < 0) return;

  int h = hour();
  int m = minute();

  if(h == feedHour && m == feedMinute)
  {
    if(!scheduleTriggered)
    {
      Serial.println("Scheduled feeding triggered");
      feedNow();
      scheduleTriggered = true;
    }
  }
  else
  {
    scheduleTriggered = false;
  }
}


void setup()
{
  Serial.begin(115200);

  Serial.println("Starting Smart Pet Feeder");

  pinMode(BUTTON_PIN,INPUT_PULLUP);

  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED3,OUTPUT);
  pinMode(LED4,OUTPUT);

  pinMode(BUZZER,OUTPUT);

  feederServo.attach(SERVO_PIN);
  feederServo.write(0);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();

  delay(2000);
  empty_bowl_weight = scale.get_units(10);

  Serial.print("Empty bowl weight: ");
  Serial.println(empty_bowl_weight);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  rtc.begin();

  while(year() < 2024)
  {
    Blynk.run();
  }

  Serial.println("RTC synced");

  timer.setInterval(1000L, checkSchedule);

  Blynk.virtualWrite(V1,255);
  Blynk.virtualWrite(V2,"System Ready");
}


void loop()
{
  Blynk.run();
  timer.run();

  if(!scale.is_ready())
  {
    Serial.println("HX711 not ready!");
    return;
  }

  if(!Blynk.connected())
  {
    Blynk.connect();
  }

  if(digitalRead(BUTTON_PIN)==LOW && !buttonPressed)
  {
    buttonPressed = true;

    Serial.println("Manual button pressed");

    feedNow();
  }

  if(digitalRead(BUTTON_PIN)==HIGH)
  {
    buttonPressed = false;
  }
}