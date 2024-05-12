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

unsigned long interval = 21600; // Feeding interval in seconds (6 hours)
int prevEpoch = 0;           

volatile int hasFedFish = 0;            // Counts the number of times the fish have been fed
volatile boolean fishFed = false;   
volatile boolean buttonPressed = false; 
volatile int servoLastVal =90;

NTPClient timeClient(ntpUDP, "ntp.bmkg.go.id", 25200, 30000); 

void feedFish()
{
  servoLastVal = myServo.read()
  myServo.write(60);          
  delayMicroseconds(500000); 
  myServo.write(90);         

  hasFedFish++;

  fishFed = true;
}

// Interrupt Service Routine (ISR) for fish button press
IRAM_ATTR void fish()
{
  buttonPressed = true;
  feedFish();
}

void setup()
{
  myServo.attach(4, 500, 2500);

  pinMode(12, INPUT_PULLUP);
  attachInterrupt(12, fish, FALLING);

  myBot.wifiConnect(ssid, pass);

  // Initialize NTP client for time synchronization
  timeClient.begin();
  timeClient.update();
  prevEpoch = timeClient.getEpochTime();

  myBot.setTelegramToken(token);

  // Send a startup message to Telegram chat if connection is successful
  if (myBot.testConnection())
  {
    myBot.sendMessage(id, "Bot dinyalakan, terhubung ke " + ssid + ", IP: " + ipToString(WiFi.localIP()));
  }

  ArduinoOTA.begin();
  ArduinoOTA.onEnd([]()
                   { myBot.sendMessage(id, "Update OTA berhasil"); });
}

void loop()
{
  ArduinoOTA.handle();

  // Send Telegram messages for feeding events and reset flags
  {
    myBot.sendMessage(id, "Kucing telah diberi makan");
    catFed = false;
  }
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
    prevEpoch = timeClient.getEpochTime();      // Update previous feeding time
    feedFish();                                 // Trigger fish feeding
    myBot.sendMessage(id, "waktunya makan"); // Send feeding notification
  }

  // Check for new Telegram messages and handle commands
  if (myBot.getNewMessage(msg))
  {
    switch (msg.text)
    {
    case "Feed fish":
      feedFish();
      break;
    case "Reset":
      prevEpoch = timeClient.getEpochTime();
      myBot.sendMessage(id, "Telah di reset");
      break;
    case "Next feed":
      int t = interval - (timeClient.getEpochTime() - prevEpoch);
      int s = t % 60;
      t = (t - s) / 60;
      int m = t % 60;
      t = (t - m) / 60;
      int h = t;
      myBot.sendMessage(id, "Dijadwalkan diberi makan dalam " + String(h) + ":" + String(m) + ":" + String(s));
      break;
    case "Info":
      myBot.sendMessage(id, "Telah memberi makan kucing " + String(hasFedCat) + " kali, dan ikan " + String(hasFedFish) + " kali. NTP: " + timeClient.getFormattedTime());
      break;
    case "Update ntp":
      myBot.sendMessage(id, "Mengupdate NTP...");
      timeClient.forceUpdate();
      break;
    default:
      myBot.sendMessage(id, "Perintah tidak dikenal");
    }
  }
}
