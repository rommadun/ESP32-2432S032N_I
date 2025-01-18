#include <TFT_eSPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <ArduinoJson.h>

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "Password"

// TFT Display
TFT_eSPI tft = TFT_eSPI();
uint16_t textColor = tft.color565(129, 139, 69); // Schriftfarbe

// NTP Zeit-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Deutsche Wochentage
const char* wochenTage[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Initialanzeige
  displayTime();
}

void loop() {
  static unsigned long lastSyncMillis = 0;
  unsigned long currentMillis = millis();

  // Überprüfen, ob Synchronisierung notwendig ist
  if (currentMillis - lastSyncMillis >= 3600000 || lastSyncMillis == 0) { // Jede Stunde
    connectWiFi();       // WLAN verbinden
    syncTime();          // Zeit synchronisieren
    disconnectWiFi();    // WLAN trennen
    lastSyncMillis = currentMillis;
  }

  // Uhrzeit und Datum anzeigen
  displayTime();
  delay(1000); // Anzeigen aktualisieren
}

void connectWiFi() {
  Serial.println("Verbinde mit WLAN...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  // Verbinden, mit Timeout von 10 Sekunden
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Verbunden!");
  } else {
    Serial.println("Verbindung fehlgeschlagen!");
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true); // WLAN trennen
  WiFi.mode(WIFI_OFF);   // WLAN-Modul deaktivieren
  Serial.println("WLAN getrennt.");
}

void syncTime() {
  timeClient.begin();
  timeClient.forceUpdate(); // Zeit synchronisieren
  adjustDST();              // Sommer-/Winterzeit anpassen
  timeClient.end();
  Serial.println("Zeit synchronisiert: " + timeClient.getFormattedTime());
}

void adjustDST() {
  time_t now = timeClient.getEpochTime();
  struct tm *ti = localtime(&now);
  int month = ti->tm_mon + 1;
  int day = ti->tm_mday;
  int weekday = ti->tm_wday;

  // Sommerzeitregelung für EU
  if ((month > 3 && month < 10) || (month == 3 && (day - weekday >= 25)) || (month == 10 && (day - weekday < 25))) {
    timeClient.setTimeOffset(7200); // CEST
  } else {
    timeClient.setTimeOffset(3600); // CET
  }
}

void displayTime() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawtime);

  String formattedTime = timeClient.getFormattedTime().substring(0, 5); // Stunden:Minuten
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d.%m.%Y", ti);
  String dateStr = String(wochenTage[ti->tm_wday]) + ", " + String(buffer);

  // Nur aktualisieren, wenn sich die Anzeige ändert
  static String lastTime = "";
  static String lastDate = "";

  if (formattedTime != lastTime || dateStr != lastDate) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(textColor, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    drawBackground(); // Hintergrund zeichnen

    // Datum und Uhrzeit anzeigen
    tft.fillRect(40, 42, 20, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe
    tft.drawString(dateStr, 160, 50, 4);
    tft.fillRect(260, 42, 20, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe

    tft.fillRect(25, 80, 270, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe
    tft.drawString(formattedTime, 160, 140, 8);
    tft.fillRect(25, 200, 270, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe

    lastTime = formattedTime;
    lastDate = dateStr;
  }
}



void drawBackground() {
  int centerX = 160;  // X-Koordinate der Mitte
  int centerY = 440;  // Y-Koordinate der Mitte (unten)
  int maxRadius = 340; // Maximale Radiusgröße
  int steps = 20;      // Anzahl der Farbverläufe

  // Startfarbe (0, 0, 0) und Endfarbe (129, 139, 69)
  uint16_t startColor = tft.color565(0, 0, 0);
  uint16_t endColor = tft.color565(40, 40, 20);

  // Berechne den Farbverlauf
  for (int i = 0; i < steps; i++) {
    // Berechne den aktuellen Radius und die aktuelle Farbe
    int radius = maxRadius - i * (maxRadius / steps);
    uint16_t color = tft.color565(
      (i * (40 / steps)), 
      (i * (40 / steps)), 
      (i * (20 / steps))
    );
    
    // Zeichne den Kreis mit dem berechneten Radius und der Farbe
    tft.fillCircle(centerX, centerY, radius, color);
  }
}
