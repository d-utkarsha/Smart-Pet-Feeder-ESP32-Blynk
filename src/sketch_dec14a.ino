#define BLYNK_TEMPLATE_ID "TMPL3HsGPQEoK"
#define BLYNK_TEMPLATE_NAME "Smart Pet Feeder"
#define BLYNK_AUTH_TOKEN "9q4_JZ4D9ESj41jSEH05WbOw480yRgTY"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

char ssid[] = "iQOO Neo9 Pro";
char pass[] = "...........";

#define SERVO_PIN 18
#define BUTTON_PIN 23

#define LED1 33
#define LED2 32
#define LED3 26
#define LED4 25
#define BUZZER 21

Servo feederServo;
WidgetRTC rtc;
BlynkTimer timer;

bool feeding = false;
bool buttonPressed = false;

int feedHour = -1;
int feedMinute = -1;

bool scheduleTriggered = false;

String lastFeedTime = "Never";

void feedNow()
{
  if(feeding) return;

  feeding = true;

  Serial.println("Feeding started");

  Blynk.virtualWrite(V2,"Feeding in progress");

  digitalWrite(BUZZER,HIGH);

  feederServo.write(60);

  for(int i=0;i<20;i++)
  {
    digitalWrite(LED1,!digitalRead(LED1));
    digitalWrite(LED2,!digitalRead(LED2));
    digitalWrite(LED3,!digitalRead(LED3));
    digitalWrite(LED4,!digitalRead(LED4));

    Blynk.run();
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