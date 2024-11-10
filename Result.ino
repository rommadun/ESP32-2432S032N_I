#include <TFT_eSPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#define WIFI_SSID "WLAN_Name"
#define WIFI_PASSWORD "WLAN_Passwort"
// Definiere die Open-Meteo API URL
#define OPEN_METEO_API_URL "api.open-meteo.com"
#define LATITUDE "12.3456"  //Hier müsst ihr die Koordinaten von eurem Ort eintragen
#define LONGITUDE "78.9012" //https://open-meteo.com/en/docs     Rechts bei Search euren Ort eintragen

// TFT Display
TFT_eSPI tft = TFT_eSPI();
uint16_t textColor = tft.color565(119, 129, 92); // Variable für Schriftfarbe

// NTP Zeit-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Wetter-API Einstellungen
const int weatherPort = 80;
float currentTemp, humidity;

// Zeit und Wetter aktualisieren
unsigned long previousMillis = 0;
const long interval = 900000; // Aktualisierung alle 15 Minuten (15*60*1000)

// Deutsche Wochentage
const char* wochenTage[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

bool syncedToday = false; // Variable für tägliche Synchronisierung

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1); // Rotation für 240x320 Display     1 = Rechts Anschluss
  tft.fillScreen(TFT_BLACK);

  // WLAN verbinden
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Verbunden!");

  // NTP Zeit-Client starten
  timeClient.begin();
  timeClient.setTimeOffset(3600); // Start mit CET (UTC + 1 Stunde)

  // Erste Wetteraktualisierung
  updateWeather();
}

void loop() {
  checkDST(); // Überprüfen und Anpassen der Sommer- und Winterzeit
  checkTimeSync();  // Synchronisiere die Uhrzeit um 4 Uhr

  // Uhrzeit und Datum anzeigen
  displayTime();

  // Wetterdaten alle 15 Minuten aktualisieren
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    updateWeather();
  }
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
    tft.fillRect(40, 22, 20, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe
    tft.drawString(dateStr, 160, 30, 4);
    tft.fillRect(260, 22, 20, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe

    // Uhrzeit anzeigen
    tft.drawString(formattedTime, 160, 95, 8);
    tft.fillRect(25, 150, 270, 4, textColor); // Strich zeichnen    Start, Positon, Länge, Dicke , Farbe


    lastTime = formattedTime;
    lastDate = dateStr;
  }

  // Wetteranzeige aktualisieren
  displayWeather();
}

void displayWeather() {
  static float lastTemp = -100.0; // Unrealistische Startwerte zum Erzwingen der ersten Anzeige
  static float lastHumidity = -1.0;
  static float tTemp = -12.3;
  static float thumidity = 100.0;

  // Prüfen, ob die Temperatur oder Luftfeuchtigkeit sich geändert hat
  if (currentTemp != lastTemp || humidity != lastHumidity) {
    // Bereich für Wetteranzeige löschen
    tft.fillRect(0, 180, 320, 50, TFT_BLACK); // Löscht den gesamten Bereich

    // Textfarbe und Ausrichtung setzen
    tft.setTextColor(textColor, TFT_BLACK);
    tft.setTextDatum(ML_DATUM); // Linksbündige Ausrichtung für die Anzeige

    // Thermometersymbol zeichnen
    drawThermometer(20, 200);
    // Temperatur mit Gradzeichen und "C" anzeigen
    tft.drawString(String(currentTemp, 1) + "", 50, 210, 6);

    // Regentropfensymbol zeichnen
    drawRaindrop(210, 200);
    // Luftfeuchtigkeit mit Prozentzeichen anzeigen
    tft.drawString(String(humidity, 0) + " ", 240, 210, 6);

    // Aktuelle Werte speichern, um sie später mit den neuen Werten zu vergleichen
    lastTemp = currentTemp;
    lastHumidity = humidity;
  }
}

void drawThermometer(int x, int y) {
  // Äußeres dickes Rechteck für den Thermometerkörper
  tft.fillRoundRect(x, y - 20, 12, 40, 6, textColor);     // Äußerer dicker Körper
  tft.fillRoundRect(x + 2, y - 18, 8, 36, 4, TFT_BLACK);  // Innere Füllung, um Dicke des Randes zu erzeugen
  // Inneres Rechteck für die Thermometersäule
  tft.fillRect(x + 5, y - 10, 2, 20, textColor); // Innerer Balken für Temperatur
  // Kreis unten für die Thermometerkugel
  tft.fillCircle(x + 6, y + 15, 10, textColor);        // Äußere Thermometerkugel
  tft.fillCircle(x + 6, y + 15, 6, TFT_BLACK);         // Innerer Kreis für dicken Rand
  // Füllung des Thermometers im Inneren der Kugel
  tft.fillCircle(x + 6, y + 15, 4, textColor);         // Zentrum der Kugel
}

void drawRaindrop(int x, int y) {
  tft.fillCircle(x, y + 8, 10, textColor);             // Vergrößerter Regentropfenkopf
  tft.fillTriangle(x - 10, y + 8, x + 10, y + 8, x, y - 12, textColor); // Vergrößerte Spitze des Regentropfens
}

void updateWeather() {
    WiFiClient client;
    if (!client.connect("api.open-meteo.com", 80)) {
        Serial.println("Verbindung zu Wetterserver fehlgeschlagen.");
        return;
    }

    // Anfrage an die Wetter-API senden
    String url = "/v1/forecast?latitude=" + String(LATITUDE) + "&longitude=" + String(LONGITUDE) + "&current=temperature_2m,relative_humidity_2m&forecast_days=1";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.open-meteo.com\r\n" +
                 "Connection: close\r\n\r\n");

    // Antwort lesen
    String response = "";
    while (client.connected() || client.available()) {
        String line = client.readStringUntil('\n'); // Zeile für Zeile lesen
        if (line.length() == 0) { // Wenn eine leere Zeile kommt, sind wir am Ende des Headers
            break;
        }
        response += line + "\n"; // Die gesamte Antwort in einer Variable speichern
    }

    // Suche den Beginn der JSON-Daten
    int jsonStart = response.indexOf('{'); // Finde den ersten '{', um den JSON-Körper zu isolieren
    if (jsonStart != -1) {
        String payload = response.substring(jsonStart); // Nimm alles ab dem ersten '{'
        payload.trim(); // Trim, um unnötige Leerzeichen zu entfernen

        Serial.println("Empfangene JSON-Antwort:");
        Serial.println(payload); // Zeigt die gesamte JSON-Antwort an

        DynamicJsonDocument doc(2048); // Größe anpassen, falls erforderlich
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Fehler beim Parsen von JSON: ");
            Serial.println(error.f_str()); // Ausgabe des Fehlertyps
            Serial.println("Payload: " + payload); // Zeigt die Payload an
            return;
        }

        // Wetterdaten extrahieren
        currentTemp = doc["current"]["temperature_2m"].as<float>();
        humidity = doc["current"]["relative_humidity_2m"].as<float>();

        // Debug-Ausgabe für die Werte
        Serial.print("Aktuelle Temperatur: ");
        Serial.println(currentTemp);
        Serial.print("Luftfeuchtigkeit: ");
        Serial.println(humidity);
    } else {
        Serial.println("JSON-Daten nicht gefunden.");
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
}
