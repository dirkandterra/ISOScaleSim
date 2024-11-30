#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WIRE Wire

long DisplayTrigger=0;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &WIRE);

void setup() {
  Serial.begin(115200);

  Serial.println("OLED FeatherWing test");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  // text display tests
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Scale 0: ");
  display.println("10000");
  display.println("Scale 1: ");
  display.println("10000");

  display.setCursor(0,0);
  display.display(); // actually display all of the above
}

void loop() {
  long now = millis();

  if(now>DisplayTrigger){
    display.display();
    DisplayTrigger=now+250;
  }
  
}
