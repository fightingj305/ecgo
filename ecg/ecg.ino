#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define ADC_RANGE 4096

#define DEAD_ZONE 512
#define CALIBRATE_LEN 15

#define AD8232_VAL 4
#define LEAD_OFF_PLUS 16
#define LEAD_OFF_MINUS 17
#define JOYSTICK_X 2 // zoom in/out
#define JOYSTICK_Y 15 // move up/down

const int j_speed = 50;

// setup 2 displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// stuff for displaying value
int heart_val, prev_heart_val, count; 
int y_offset, screen_range;
int X_CALIBRATE, Y_CALIBRATE;
char val_buffer[20];
char lop_buffer[20];
char lom_buffer[20];
bool lead_off_plus, lead_off_minus;

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

  // calibrate the (incredibly drifty) joystick
  for (int i = 0; i < CALIBRATE_LEN; i++){
    X_CALIBRATE += analogRead(JOYSTICK_X);
    Y_CALIBRATE += analogRead(JOYSTICK_Y);
  }
  X_CALIBRATE /= CALIBRATE_LEN;
  Y_CALIBRATE /= CALIBRATE_LEN;

  y_offset = 0;
  screen_range = 4096;
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

void draw_display2(){
  // copy the value to a char buffer to display
  display2.clearDisplay();
  snprintf(val_buffer, 12,"H_VAL: %d", heart_val);
  snprintf(lop_buffer, 8,"LO+: %d", lead_off_plus);
  snprintf(lom_buffer, 8,"LO-: %d", lead_off_minus);

  // draw 3 buffers for display 2
  display2.setCursor(0, 0);
  display2.print(val_buffer);
  display2.setCursor(0, 20);
  display2.print(lop_buffer);
  display2.setCursor(0, 40);
  display2.print(lom_buffer);
}

int adjust_val (int val, int range, int y_offset) {
  return ((val - y_offset) * SCREEN_HEIGHT) / range;
}

// returns whether joystick was hit
bool update_bounds(int jx, int jy) {
  int initial_sr = screen_range;
  int initial_yo = y_offset;
  if (jx > X_CALIBRATE + DEAD_ZONE) { // zoom out
    screen_range+=j_speed;
  }
  else if (jx < X_CALIBRATE - DEAD_ZONE) { // zoom in
    screen_range-=j_speed;
  }

  if (jy > Y_CALIBRATE + DEAD_ZONE) { // scroll up
    y_offset-=j_speed;
  }
  else if (jy < Y_CALIBRATE - DEAD_ZONE) { // scroll down
    y_offset+=j_speed;
  }
  screen_range = min(screen_range, ADC_RANGE);
  screen_range = max(screen_range, SCREEN_HEIGHT);
  y_offset = max(y_offset, 0);
  y_offset =  min(y_offset, ADC_RANGE - screen_range);
  return y_offset != initial_yo || screen_range != initial_sr;
}

void loop() {
  int joy_x = analogRead(JOYSTICK_X);
  int joy_y = analogRead(JOYSTICK_Y);
  Serial.printf("%d %d %d %d \n", joy_x, joy_y, screen_range, y_offset);
  bool update = update_bounds(joy_x, joy_y);
  draw_wave(count == SCREEN_WIDTH, adjust_val(heart_val, screen_range, y_offset), adjust_val(prev_heart_val, screen_range, y_offset), &count);
  // update values
  prev_heart_val = heart_val;
  heart_val = analogRead(AD8232_VAL);
  lead_off_plus = digitalRead(LEAD_OFF_PLUS);
  lead_off_minus = digitalRead(LEAD_OFF_MINUS);

  draw_display2();

  // call display
  display.display();
  display2.display();
}
