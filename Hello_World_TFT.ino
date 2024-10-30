#include <TFT_eSPI.h>  // TFT-Bibliothek für ST7789-Treiber verwenden
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Erstelle ein TFT-Objekt

void setup() {
  tft.init();               // Initialisiert das Display
  tft.setRotation(1);       // Dreht das Display, falls notwendig (0–3 ausprobieren)
  tft.fillScreen(TFT_BLACK); // Füllt den Hintergrund mit Schwarz
}

void loop() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Textfarbe Weiß, Hintergrundfarbe Schwarz
  tft.setTextSize(2);                      // Textgröße einstellen
  tft.setCursor(10, 10);                   // Startposition für den Text
  tft.println("Hello World!");             // "Hello World" ausgeben
  delay(1000);                             // 1 Sekunde warten
}
