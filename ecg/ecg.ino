#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define ADC_RANGE 1024

#define AD8232_VAL 4
#define LEAD_OFF_PLUS 16
#define LEAD_OFF_MINUS 17

// setup 2 displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// stuff for displaying value
int heart_val, prev_heart_val, count;
char val_buffer[3];

void setup() {
  Serial.begin(115200);
  // manually reconfigured D/C# pin on one OLED to have I2C addr 0x3D
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  if(!display2.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  pinMode(LEAD_OFF_PLUS, INPUT);
  pinMode(LEAD_OFF_MINUS, INPUT);

  display.clearDisplay();
  display2.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  display2.setCursor(0, 0);

  display.display(); 
  display2.display(); 

  val_buffer[2] = '\0';
}

// very simple funtion to just draw lines between adj data pts
void draw_wave(bool reset, int data, int last_data, int *counter) {
  if (reset) {
    *counter = 0;
    display.clearDisplay();
  }
  else {
    display.drawLine(*counter - 1, last_data, *counter, data, WHITE);
    *counter = *counter + 1;
  }
}

void loop() {
  draw_wave(count == SCREEN_WIDTH, heart_val, prev_heart_val, &count);
  // update values
  prev_heart_val = heart_val;
  heart_val = analogRead(AD8232_VAL);
  heart_val *= (SCREEN_HEIGHT/ADC_RANGE);
  display.display();
  // copy the value to a char buffer to display
  display2.clearDisplay();
  snprintf(val_buffer, 2, "%d", heart_val);
  display2.setCursor(0, 0);
  display2.print(val_buffer);
  display2.display();
}
