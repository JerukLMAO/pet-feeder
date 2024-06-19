#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <CTBot.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>

TBMessage msg; // Stores Telegram message data
CTBot myBot;
WiFiUDP ntpUDP;
Servo myServo; // Servo object for controlling the fish feeder mechanism
WiFiClient wifiClient;

String ssid = "Fadgib";         // Your Wi-Fi network name (SSID)
String pass = "fadhligibran12"; // Your Wi-Fi network password

String token = "6526441782:AAFJM0zrJW1GmXOcQazzifJE3h-oxG7k8Hg"; // Your Telegram bot token
const int id = 1221284266;                                       // Telegram chat ID where messages will be sent

unsigned long interval = 86400; // Feeding interval in seconds (24 hours)
int prevEpoch = 0;

volatile int hasFedFish = 0; // Counts the number of times the fish have been fed
volatile boolean fishFed = false;
volatile boolean buttonPressed = false;
volatile int timesPressed = 0;
volatile int servoPos = 90;
volatile boolean servoDirection = 1; // 1 = Forward   0 = Backward

NTPClient timeClient(ntpUDP, "ntp.bmkg.go.id", 25200, 30000);

void feedFish()
{
  if (myServo.read() == 0 || myServo.read() == 90)
  {
    servoDirection = 1; // Forward
  }
  else if (myServo.read() == 180)
  {
    servoDirection = 0; // Backward
  }

  if (servoDirection)
  {
    servoPos = myServo.read() + 10;
  }
  else
  {
    servoPos = myServo.read() - 10;
  }

  constrain(servoPos, 0, 180);
  myServo.write(servoPos);

  hasFedFish++;
  fishFed = true;
}

// Interrupt Service Routine (ISR) for fish button press
IRAM_ATTR void fish()
{
  timesPressed++;
  delayMicroseconds(20e4);
  if (timesPressed >= 3)
  {
    buttonPressed = true;
    feedFish();
    timesPressed = 0;
  }
}

void setup()
{
  myServo.attach(5, 500, 2500);

  pinMode(14, INPUT_PULLUP);
  attachInterrupt(14, fish, FALLING);

  myBot.wifiConnect(ssid, pass);

  // Initialize NTP client for time synchronization
  timeClient.begin();
  timeClient.update();
  prevEpoch = timeClient.getEpochTime();

  myBot.setTelegramToken(token);

  // Send a startup message to Telegram chat if connection is successful
  if (myBot.testConnection())
  {
    myBot.sendMessage(id, "Bot dinyalakan, terhubung ke " + ssid);
  }

  ArduinoOTA.begin();
  ArduinoOTA.onEnd([]()
                   { myBot.sendMessage(id, "Update OTA berhasil"); });
}

void loop()
{
  ArduinoOTA.handle();

  // Send Telegram messages for feeding events and reset flags
  if (fishFed)
  {
    myBot.sendMessage(id, "Ikan telah diberi makan");
    fishFed = false;
  }
  if (buttonPressed)
  {
    myBot.sendMessage(id, "Tombol ditekan ");
    buttonPressed = false;
  }

  // Check for scheduled feeding time based on interval
  if (timeClient.getEpochTime() - prevEpoch >= interval)
  {
    prevEpoch = timeClient.getEpochTime();   // Update previous feeding time
    feedFish();                              // Trigger fish feeding
    myBot.sendMessage(id, "waktunya makan"); // Send feeding notification
  }

  // Check for new Telegram messages and handle commands
  if (myBot.getNewMessage(msg))
  {
    if (msg.text == "Feed fish")
    {
      feedFish();
    }

    if (msg.text == "Reset jadwal")
    {
      prevEpoch = timeClient.getEpochTime();
      myBot.sendMessage(id, "Telah di reset");
    }

    // Calculate and send time remaining for next scheduled feeding
    if (msg.text == "Next feed")
    {
      int t = interval - (timeClient.getEpochTime() - prevEpoch);
      int s = t % 60;
      t = (t - s) / 60;
      int m = t % 60;
      t = (t - m) / 60;
      int h = t;
      myBot.sendMessage(id, "Dijadwalkan diberi makan dalam " + String(h) + ":" + String(m) + ":" + String(s));
    }

    // Send status information (feeding counts and NTP time)
    if (msg.text == "Info")
    {
      myBot.sendMessage(id, "Telah memberi makan ikan " + String(hasFedFish) + " kali. NTP: " + timeClient.getFormattedTime());
    }

    if (msg.text == "Update ntp")
    {
      myBot.sendMessage(id, "Mengupdate NTP...");
      timeClient.forceUpdate();
    }
  }
}
