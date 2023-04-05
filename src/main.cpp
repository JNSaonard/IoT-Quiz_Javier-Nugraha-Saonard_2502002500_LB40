#include <Arduino.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DHT_U.h>
#include "BH1750.h"
#include "DHTesp.h"


#if defined(ESP32)
#include <WiFi.h>
#endif

#define LED_RED 13
#define LED_GREEN 12
#define LED_YELLOW 23
#define PIN_SCL 34
#define PIN_DHT 39
#define PIN_SDA 27
#define PIN_SW 17

#if defined(ESP32)
#define LED_COUNT 3
const uint8_t arLed[LED_COUNT] = {LED_RED, LED_GREEN, LED_YELLOW};
#endif

const char *ssid = "Lab-Eng";
const char *password = "Lab-Eng123!";

#define MQTT_BROKER "broker.emqx.io"
#define MQTT_TOPIC_LUX "esp32_javier/data/lux"
#define MQTT_TOPIC_HUMID "esp32_javier/data/humid"
#define MQTT_TOPIC_TEMP "esp32_javier/data/temp"
#define MQTT_TOPIC_PUBLISH "esp32_javier/data"
#define MQTT_TOPIC_SUBSCRIBE "esp32_javier/cmd/#"

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Ticker timerPublishTemp, timerPublishHumid, timerPublishLux, ledOff;
DHTesp dht;
BH1750 lightMeter;

char g_szDeviceId[30];
void WifiConnect();
boolean mqttConnect();
    void onPublishMessageTemp();
    void onPublishMessageHumid();
    void onPublishMessageLux();

void loop(){
  mqtt.loop();
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
  String strTopic = topic;
  int8_t idx = strTopic.lastIndexOf('/') + 1;
  String strDev = strTopic.substring(idx);
  Serial.printf("==> Recv [%s]: ", topic);
  Serial.write(payload, len);
  Serial.println();
  if (strncmp((char*)payload, "led on", len)==0)
  {
    if (strDev == "LedGreen")
      digitalWrite(LED_GREEN, HIGH);
  }
  else if (strncmp((char*)payload, "led off", len) ==0)
  {
    if (strDev == "LedGreen")
      digitalWrite(LED_GREEN, LOW);
  }
}
void onPublishMessageTemp()
{
  char szTopic[50];
  char szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
  float temperature = dht.getTemperature();
  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    Serial.printf("Temperature: %.2f C\n", temperature);
    sprintf(szTopic, "%s/temp", MQTT_TOPIC_TEMP);
    sprintf(szData, "%.2f", temperature);
    mqtt.publish(MQTT_TOPIC_TEMP, szData);
  }
  else
    Serial.printf("\n");
}

void onPublishMessageHumid()
{
  char szTopic[50];
  char szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
  float humidity = dht.getHumidity();

  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    Serial.printf("Humidity: %.2f %%\n",humidity);
    sprintf(szTopic, "%s/humidity", MQTT_TOPIC_HUMID);
    sprintf(szData, "%.2f", humidity);
    mqtt.publish(MQTT_TOPIC_HUMID, szData);
  }
  else
    Serial.printf("\n");
}

void onPublishMessageLux()
{
  char szTopic[50];
  char szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
  float lux = lightMeter.readLightLevel();

  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    Serial.printf("light: %.2f\n", lux);
  }
  else
    Serial.printf("Light: %.2f lux\n", lux);
    sprintf(szTopic, "%s/light", MQTT_TOPIC_PUBLISH);
    sprintf(szData, "%.2f", lux);
    mqtt.publish(MQTT_TOPIC_PUBLISH, szData);
    ledOff.once_ms(100, []()
    { digitalWrite(LED_BUILTIN, LOW); });


  if (lightMeter.readLightLevel() < 400)
  {
    Serial.printf("Safedoor Closed = %.2f lux\n", lux);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }
  else if (lightMeter.readLightLevel() > 400)
  {
    Serial.printf("WARNING Safedoor Opened = %.2f lux\n", lux);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
  }
}

void setup()
{
  Serial.begin(9600);
  delay(100);
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t i = 0; i < LED_COUNT; i++)
    pinMode(arLed[i], OUTPUT);
  pinMode(PIN_SW, INPUT);

  dht.setup(PIN_DHT, DHTesp::DHT11);
  Wire.begin(PIN_SDA, PIN_SCL);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);

  Serial.printf("Free Memory: %d\n", ESP.getFreeHeap());
  WifiConnect();
  mqttConnect();

    #if defined(ESP32)
  timerPublishTemp.attach_ms(5000, onPublishMessageTemp);
  timerPublishHumid.attach_ms(7000, onPublishMessageHumid);
  timerPublishLux.attach_ms(4000, onPublishMessageLux);
    #endif
}

boolean mqttConnect()
{
#if defined(ESP32)
  sprintf(g_szDeviceId, "esp32_%08X", (uint32_t)ESP.getEfuseMac());
#endif
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCallback);
  Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);
  boolean status = mqtt.connect(g_szDeviceId);
  if (status == false)
  {
    Serial.print(" fail, rc=");
    Serial.print(mqtt.state());
    return false;
  }
  Serial.println(" success");
  mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
  Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
  onPublishMessageTemp();
  onPublishMessageHumid();
  onPublishMessageLux();

  return mqtt.connected();
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}