#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Define pins for LED indicators
#define FA 25
#define GA 26
#define HA 27

// Define TFT display pins
#define TFT_CS     5   // Chip select pin
#define TFT_RST    4   // Reset pin
#define TFT_DC     13  // Data/Command pin

// Wi-Fi credentials
const char* ssid = "Sense Semi";        // WiFi SSID
const char* password = "SSIT@504";      // WiFi Password

// Cloud server URL (Update with your cloud server endpoint)
const char* serverUrl = "https://hackthon.sensesemi.in/air-system/update";

// DHT11 sensor setup
#define DHTPIN 32                       // Pin connected to the DHT11 sensor
#define DHTTYPE DHT11                   // Type of DHT sensor (DHT11)
DHT dht(DHTPIN, DHTTYPE);               // Create DHT object

// TFT display object creation
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Timing variables
unsigned long lastTime = 0;             // Last time data was updated
const unsigned long updateInterval = 30 * 1000; // 30 seconds interval for updates
int currentSlide = 0;                   // Variable to track the current slide for the display

void setup() {
  Serial.begin(9600);

  // Initialize pins for LEDs as OUTPUT
  pinMode(FA, OUTPUT);
  pinMode(GA, OUTPUT);
  pinMode(HA, OUTPUT);

  // Initialize TFT display
  tft.begin();
  tft.setRotation(3);   // Set display rotation (landscape mode)
  tft.fillScreen(ILI9341_BLACK);   // Clear the screen
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("TFT Initialized");

  // Initialize DHT11 sensor
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    tft.fillScreen(ILI9341_BLACK);  // Clear screen for update
    tft.setCursor(10, 40);
    tft.setTextColor(ILI9341_RED);
    tft.println("Connecting WiFi...");
    delay(500);
  }

  // Display Wi-Fi connection status on TFT
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 40);
  tft.setTextColor(ILI9341_GREEN);
  tft.println("WiFi Connected!");
  Serial.println("Connected to WiFi");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastTime > updateInterval) {
    lastTime = currentTime;  // Update last time variable

    // Air quality monitoring (using MQ135 sensor)
    int mq135Value = analogRead(34);  // Read analog value from MQ135 sensor
    int airQualityPercentage = map(mq135Value, 0, 2500, 0, 100);  // Map value to percentage
    airQualityPercentage = constrain(airQualityPercentage, 0, 100);  // Constrain between 0 and 100

    // Read temperature and humidity from DHT11 sensor
    float temperature = dht.readTemperature();  // Read temperature in Celsius
    float humidity = dht.readHumidity();        // Read humidity in percentage

    // Slide mechanism for TFT display
    tft.fillScreen(ILI9341_BLACK);  // Clear the screen for new data

    switch (currentSlide) {
      case 0:  // Display air quality, temperature, and humidity on the first slide
        tft.setCursor(10, 20);
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_CYAN);
        tft.print("Air Quality: ");
        tft.print(airQualityPercentage);
        tft.print("%");

        tft.setCursor(10, 60);
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("Temp: ");
        tft.print(temperature);
        tft.print(" C");

        tft.setCursor(10, 100);
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_BLUE);
        tft.print("Humidity: ");
        tft.print(humidity);
        tft.print(" %");

        // LED indicators for air quality levels
        tft.setCursor(10, 140);
        if (airQualityPercentage < 60) {  // Healthy air
          digitalWrite(FA, HIGH);
          digitalWrite(GA, LOW);
          digitalWrite(HA, LOW);
          tft.setTextColor(ILI9341_GREEN);
          tft.print("Healthy Air");
        } else if (airQualityPercentage >= 60 && airQualityPercentage < 90) {  // Good air
          digitalWrite(FA, LOW);
          digitalWrite(GA, HIGH);
          digitalWrite(HA, LOW);
          tft.setTextColor(ILI9341_YELLOW);
          tft.print("Good Air");
        } else {  // Unhealthy air
          digitalWrite(FA, LOW);
          digitalWrite(GA, LOW);
          digitalWrite(HA, HIGH);
          tft.setTextColor(ILI9341_RED);
          tft.print("UnHealthy Air");
        }
        break;

      case 1:  // Display precautionary measures when air quality is hazardous
        if (airQualityPercentage >= 90) {  // Show precautions for unhealthy air
          tft.setCursor(10, 20);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_RED);
          tft.println("Precautions!");

          tft.setCursor(10, 60);
          tft.setTextSize(2);
          tft.setTextColor(ILI9341_MAGENTA);
          tft.println("1. Stay indoors.");
          tft.setCursor(10, 90);
          tft.println("2. Use air purifiers.");
          tft.setCursor(10, 120);
          tft.println("3. Wear masks outside.");
          tft.setCursor(10, 150);
          tft.println("4. Avoid intense exercise.");
          tft.setCursor(10, 180);
          tft.println("5. Close windows & doors.");
        } else {  // Display a message when air quality is fine
          tft.setCursor(10, 60);
          tft.setTextSize(2);
          tft.setTextColor(ILI9341_GREEN);
          tft.println("Air quality is good.");
        }
        break;
    }

    // Cycle between two slides
    currentSlide = (currentSlide + 1) % 2;

    // Push data to cloud server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);  // Initialize HTTP connection
      http.addHeader("Content-Type", "application/json");  // Set content type as JSON

      // Create JSON payload for data transmission
      String jsonPayload = "{";
      jsonPayload += "\"appId\":\"maneeesh\","; 
      jsonPayload += "\"quality\":" + String(airQualityPercentage) + ",";
      jsonPayload += "\"temp\":" + String(temperature) + ",";
      jsonPayload += "\"humidity\":" + String(humidity);
      jsonPayload += "}";

      // Send HTTP POST request
      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        Serial.println("Data sent to cloud successfully.");
        Serial.println(http.getString());  // Print server response
      } else {
        Serial.println("Error sending data to cloud.");
        Serial.println(httpResponseCode);
      }

      // End HTTP connection
      http.end();
    } else {
      Serial.println("WiFi Disconnected, cannot send data.");
    }
  }
}
