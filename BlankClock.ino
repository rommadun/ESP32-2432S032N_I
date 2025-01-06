#include <TFT_eSPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <ArduinoJson.h>

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "Password"

// TFT Display
TFT_eSPI tft = TFT_eSPI();
uint16_t textColor = tft.color565(129, 139, 69); // Variable für Schriftfarbe Sake:     119, 129, 92

// NTP Zeit-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Zeit und Wetter aktualisieren
unsigned long previousMillis = 0;
const long interval = 900000; //900000// Aktualisierung alle 15 Minuten (15*60*1000) Debug: 30000

// Deutsche Wochentage
String GlobalDay;
const char* wochenTage[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

bool syncedToday = false; // Variable für tägliche Synchronisierung

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1); // Rotation für 240x320 Display     1 = Rechts Anschluss
  tft.fillScreen(TFT_BLACK);

  // WLAN verbinden
  //WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Verbunden!");

  // NTP Zeit-Client starten
  timeClient.begin();
  timeClient.setTimeOffset(3600); // Start mit CET (UTC + 1 Stunde)
}


void loop() {
  checkDST(); // Überprüfen und Anpassen der Sommer- und Winterzeit
  checkTimeSync();  // Synchronisiere die Uhrzeit um 4 Uhr

  // Uhrzeit und Datum anzeigen
  displayTime();
}


void checkDST() {
  // Hole die aktuelle Zeit und extrahiere den Monat und den Tag
  time_t now = timeClient.getEpochTime();
  struct tm *ti = localtime(&now);
  int month = ti->tm_mon + 1; // tm_mon ist 0-basiert, daher +1
  int day = ti->tm_mday;
  int weekday = ti->tm_wday;  // 0 = Sonntag, 1 = Montag, ..., 6 = Samstag

  // Sommerzeitregelung für EU: Letzter Sonntag im März bis letzter Sonntag im Oktober
  if ((month > 3 && month < 10) ||             // Monate April bis September
      (month == 3 && (day - weekday >= 25)) || // Letzter Sonntag im März
      (month == 10 && (day - weekday < 25))) { // Letzter Sonntag im Oktober
    timeClient.setTimeOffset(7200);  // CEST (UTC + 2 Stunden)
  } else {
    timeClient.setTimeOffset(3600);  // CET (UTC + 1 Stunde)
  }
}


int lastSyncHour = -1;
void checkTimeSync() {
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Synchronisiere nur zu jeder vollen Stunde und einmal pro Stunde
  if (currentMinute == 0 && currentHour != lastSyncHour) {
    timeClient.forceUpdate();  // Zwinge eine Synchronisierung
    lastSyncHour = currentHour; // Aktualisiere letzte Synchronisierungsstunde
    Serial.println("Uhrzeit synchronisiert zur vollen Stunde.");
  }
}


void displayTime() { 
  //tft.loadFont("FreeMono");
  String formattedTime = timeClient.getFormattedTime().substring(0, 5); // Stunden:Minuten
  String dateStr = getFormattedDate();
  //String dateStr = "-- " + getFormattedDate() + " --";

  // Bereich für Datum und Uhrzeit leeren und neu zeichnen
  static String lastTime = "";
  static String lastDate = "";

  if (formattedTime != lastTime || dateStr != lastDate) {
    tft.fillRect(0, 0, 320, 160, TFT_BLACK); // Bereich leeren

    // Datum anzeigen
    tft.setTextColor(textColor, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.fillRect(40, 32, 20, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe
    tft.drawString(dateStr, 160, 40, 4);
    tft.fillRect(260, 32, 20, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe
    //tft.drawString(GlobalDay, 160, 60, 6);

    // Uhrzeit anzeigen
    tft.fillRect(25, 80, 270, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe
    tft.drawString(formattedTime, 160, 140, 8);
    tft.fillRect(25, 200, 270, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe

    lastTime = formattedTime;
    lastDate = dateStr;
  }
}


String getFormattedDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime(&rawtime);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d.%m.%Y", ti); // Datum formatieren

  // Wochentag hinzufügen
  String dayOfWeek = String(wochenTage[ti->tm_wday]);
  return dayOfWeek + ", " + String(buffer);
  //GlobalDay = String(wochenTage[ti->tm_wday]);
  //return String(buffer);
}
