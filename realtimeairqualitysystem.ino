#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// =====================================================
// WIFI
// =====================================================

const char* ssid = "Airtel_bhar_1192";
const char* password = "air12509";

// =====================================================
// THINGSPEAK
// =====================================================

const char* WRITE_API_KEY = "O7RDEG7KOEI9NRZS";

// =====================================================
// PIN CONNECTIONS
// =====================================================

#define DHT_PIN     4
#define DHT_TYPE    DHT22

#define MQ135_PIN   34
#define MQ7_PIN     35

#define RELAY_PIN   18
#define BUZZER_PIN  19

DHT dht(DHT_PIN, DHT_TYPE);

// =====================================================
// THRESHOLDS
// =====================================================

// Buzzer starts when either sensor reaches this value
const int BUZZER_THRESHOLD = 1000;

// Fan starts when either sensor reaches this value
const int FAN_THRESHOLD = 1500;

// =====================================================
// THINGSPEAK TIMER
// =====================================================

unsigned long lastThingSpeakUpdate = 0;

const unsigned long THINGSPEAK_INTERVAL = 20000;


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  dht.begin();

  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ7_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  // ---------------------------------------------------
  // IMPORTANT:
  // Relay is ACTIVE-LOW.
  //
  // HIGH = OFF
  // LOW  = ON
  // ---------------------------------------------------

  digitalWrite(RELAY_PIN, HIGH);
  pinMode(RELAY_PIN, OUTPUT);

  // Buzzer OFF
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" REAL-TIME AIR QUALITY MONITOR");
  Serial.println("======================================");

  // ===================================================
  // WIFI CONNECTION
  // ===================================================

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Fan: OFF");
  Serial.println("Buzzer: OFF");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  // ===================================================
  // READ DHT22
  // ===================================================

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();


  // ===================================================
  // READ MQ SENSORS
  // ===================================================

  int mq135 = analogRead(MQ135_PIN);
  int mq7 = analogRead(MQ7_PIN);


  // ===================================================
  // DHT ERROR CHECK
  // ===================================================

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("DHT22 ERROR!");

    delay(2000);

    return;
  }


  // ===================================================
  // DEFAULT STATE
  // ===================================================

  bool fan = false;
  bool buzzer = false;

  String status = "NORMAL";
  String cause = "Air Quality Normal";


  // ===================================================
  // CRITICAL CONDITION
  // FAN + BUZZER ON
  // ===================================================

  if (mq135 >= FAN_THRESHOLD && mq7 >= FAN_THRESHOLD)
  {
    fan = true;
    buzzer = true;

    status = "CRITICAL";
    cause = "High Air Pollution + High CO";
  }

  else if (mq135 >= FAN_THRESHOLD)
  {
    fan = true;
    buzzer = true;

    status = "CRITICAL";
    cause = "High Air Pollution";
  }

  else if (mq7 >= FAN_THRESHOLD)
  {
    fan = true;
    buzzer = true;

    status = "CRITICAL";
    cause = "High CO Level";
  }


  // ===================================================
  // WARNING CONDITION
  // BUZZER ONLY
  // ===================================================

  else if (mq135 >= BUZZER_THRESHOLD && mq7 >= BUZZER_THRESHOLD)
  {
    fan = false;
    buzzer = true;

    status = "WARNING";
    cause = "Elevated Air Pollution + CO";
  }

  else if (mq135 >= BUZZER_THRESHOLD)
  {
    fan = false;
    buzzer = true;

    status = "WARNING";
    cause = "Elevated Air Pollution";
  }

  else if (mq7 >= BUZZER_THRESHOLD)
  {
    fan = false;
    buzzer = true;

    status = "WARNING";
    cause = "Elevated CO Level";
  }


  // ===================================================
  // NORMAL CONDITION
  // ===================================================

  else
  {
    fan = false;
    buzzer = false;

    status = "NORMAL";
    cause = "Air Quality Normal";
  }


  // ===================================================
  // BUZZER CONTROL
  // ===================================================

  if (buzzer == true)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
  }


  // ===================================================
  // FAN / RELAY CONTROL
  //
  // ACTIVE-LOW RELAY
  //
  // Fan OFF → HIGH
  // Fan ON  → LOW
  // ===================================================

  if (fan == true)
  {
    digitalWrite(RELAY_PIN, LOW);
  }
  else
  {
    digitalWrite(RELAY_PIN, HIGH);
  }


  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println();
  Serial.println("----------------------------------------");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("MQ-135 Raw Value: ");
  Serial.println(mq135);

  Serial.print("MQ-7 Raw Value: ");
  Serial.println(mq7);

  Serial.print("Buzzer: ");

  if (buzzer)
    Serial.println("ON");
  else
    Serial.println("OFF");

  Serial.print("Fan: ");

  if (fan)
    Serial.println("ON");
  else
    Serial.println("OFF");

  Serial.print("STATUS: ");
  Serial.println(status);

  Serial.print("CAUSE: ");
  Serial.println(cause);


  // ===================================================
  // SEND TO THINGSPEAK
  // ===================================================

  if (millis() - lastThingSpeakUpdate >= THINGSPEAK_INTERVAL ||
      lastThingSpeakUpdate == 0)
  {
    lastThingSpeakUpdate = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;

      String url =
        "http://api.thingspeak.com/update?api_key=" +
        String(WRITE_API_KEY) +

        "&field1=" +
        String(temperature, 2) +

        "&field2=" +
        String(humidity, 2) +

        "&field3=" +
        String(mq135) +

        "&field4=" +
        String(mq7) +

        "&field5=" +
        String(fan ? 1 : 0) +

        "&field6=" +
        String(buzzer ? 1 : 0);


      http.begin(url);

      int responseCode = http.GET();

      Serial.print("ThingSpeak Response: ");
      Serial.println(responseCode);

      if (responseCode == 200)
      {
        Serial.println(
          "Data successfully sent to ThingSpeak!"
        );
      }
      else
      {
        Serial.println(
          "ThingSpeak upload failed."
        );
      }

      http.end();
    }
    else
    {
      Serial.println("WiFi disconnected!");
    }
  }


  delay(2000);
}