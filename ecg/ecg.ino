#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h> 

#include <WiFiUdp.h> 
#include <HTTPClient.h>
#include <WiFi.h>

#include "credentials.h"

#define LONG_MAX 2147483647

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define ADC_RANGE 4096

#define DEAD_ZONE 512
#define CALIBRATE_LEN 15

#define DEB_DELAY 10
#define COLLECT_TIME_MS 3000
#define COLLECT_BUF_SIZE 80

#define RESPONSE_TIMEOUT 5000

// pins
#define AD8232_PIN 33
#define LEAD_OFF_PLUS 16
#define LEAD_OFF_MINUS 17
#define JOYSTICK_X 32 // zoom in/out
#define JOYSTICK_Y 35 // move up/down
#define ENABLE_BUT 26
#define SEED_PIN 36

// wifi stufff
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWORD;
HTTPClient http;

// setup 2 displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// sensor values
int heart_val, prev_heart_val, count; 
bool lead_off_plus, lead_off_minus;

// scroll variables
const int j_speed = 50;
int y_offset, screen_range;
int X_CALIBRATE, Y_CALIBRATE;
bool last_stopped;

// buffers for idsplay
char val_buffer[20];
char lop_buffer[20];
char lom_buffer[20];
int heart_buffer[SCREEN_WIDTH];
int collect_buffer[COLLECT_BUF_SIZE][2];

bool enable = true;
bool deb_timer_on;
bool last_but_state = true;
bool collect_timer_on = false;
bool last_collect_state = false;
unsigned long long int debounce_timer, collect_timer;

int values_collected = 0;
String influx_addr = INFLUX_ADDR;
String influx_token = INFLUX_TOKEN;


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
  
  connect_wifi();

  pinMode(LEAD_OFF_PLUS, INPUT);
  pinMode(LEAD_OFF_MINUS, INPUT);
  pinMode(ENABLE_BUT, INPUT_PULLUP);

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
  debounce_timer = millis();
  
  randomSeed(analogRead(SEED_PIN));
}

void connect_wifi() {
  WiFi.begin(ssid, password);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    
  }
}

// very simple funtion to just draw lines between adj data pts
void draw_wave(bool reset, bool stopped, bool prev_stop, int buf[SCREEN_WIDTH], int *counter) {
  if (reset || !stopped && prev_stop) {
    *counter = 0;
    display.clearDisplay();
  }
  else if (!stopped){
    int val = buf[*counter];
    int prev_val = val;
    if (*counter > 0) { 
      prev_val = buf[*counter - 1];
    }
    display.drawLine(*counter - 1,  adjust_val(prev_val, screen_range, y_offset), *counter,  adjust_val(val, screen_range, y_offset), WHITE);
    *counter = *counter + 1;
  }
  else {
    display.clearDisplay();
    for (int i = 0; i < *counter-1; i++) {
      display.drawLine(i,  adjust_val(buf[i], screen_range, y_offset), i+1, adjust_val(buf[i+1], screen_range, y_offset), WHITE);
    }
  }
}

void draw_display2(){
  // copy the value to a char buffer to display
  display2.clearDisplay();
  snprintf(val_buffer, 16,"SENDING DATA: %d", enable);
  snprintf(lop_buffer, 8,"LO+: %d", lead_off_plus);
  snprintf(lom_buffer, 8,"LO-: %d", lead_off_minus);

  // draw 3 buffers for display 2
  const char *wifi_status = WiFi.status() == WL_CONNECTED ? "WiFi Connected" : "No WiFi";
  display2.setCursor(0,20);
  display2.print(wifi_status);
  display2.setCursor(0, 30);
  display2.print(val_buffer);
  display2.setCursor(0, 40);
  display2.print(lop_buffer);
  display2.setCursor(0, 50);
  display2.print(lom_buffer);
}

int adjust_val (int val, int range, int y_offset) {
  return ((val - y_offset) * SCREEN_HEIGHT) / range;
}

// returns whether joystick was hit
bool update_bounds(int jx, int jy) {
  int initial_sr = screen_range;
  int initial_yo = y_offset;
  if (jx > X_CALIBRATE + DEAD_ZONE) { // zoom in
    screen_range-=j_speed;
  }
  else if (jx < X_CALIBRATE - DEAD_ZONE) { // zoom out
    screen_range+=j_speed;
  }

  if (jy > Y_CALIBRATE + DEAD_ZONE) { // scroll down
    y_offset+=j_speed;
  }
  else if (jy < Y_CALIBRATE - DEAD_ZONE) { // scroll up
    y_offset-=j_speed;
  }
  screen_range = min(screen_range, ADC_RANGE);
  screen_range = max(screen_range, SCREEN_HEIGHT);
  y_offset = max(y_offset, 0);
  y_offset =  min(y_offset, ADC_RANGE - screen_range);
  return y_offset != initial_yo || screen_range != initial_sr;
}

void post_data(int item_count, long collection_id){
  http.begin(influx_addr + "/api/v2/write?org=ECG+Data&bucket=data");
  http.addHeader("Authorization", "Token "+influx_token);
  http.addHeader("Content-Type", " text/plain; charset=utf-8");
  String data;
  for (int i = 0; i < min(item_count, COLLECT_BUF_SIZE); i++) {
    data = "ecg_data adc_value="+String(collect_buffer[i][0])+"u,collection_time="+String(collect_buffer[i][1])+"u,collection_id="+String(collection_id)+"u";
    int http_response = http.POST(data);
  }
  http.end();
}

void loop() {
  // handle zoom
  int joy_x = analogRead(JOYSTICK_X);
  int joy_y = analogRead(JOYSTICK_Y);
  bool update = update_bounds(joy_x, joy_y);
  bool read_but = digitalRead(ENABLE_BUT);

  // update values
  heart_buffer[count] = heart_val;
  draw_wave(count == SCREEN_WIDTH, update, last_stopped, heart_buffer, &count);
  unsigned long long read_time = millis();
  heart_val = analogRead(AD8232_PIN);
  lead_off_plus = digitalRead(LEAD_OFF_PLUS);
  lead_off_minus = digitalRead(LEAD_OFF_MINUS);
  last_stopped = update;
  draw_display2();
  
  // debounce button
  if (read_but && !last_but_state && !deb_timer_on) {
    debounce_timer = millis();
    deb_timer_on = true;
  }
  if (deb_timer_on && millis() - debounce_timer > DEB_DELAY) {
    if (read_but && !collect_timer_on) {
      collect_timer_on = true;
      collect_timer = millis();
      values_collected = 0;
    }
    deb_timer_on = false;
  }
  last_but_state = read_but;

  if (collect_timer_on && read_time < COLLECT_TIME_MS + collect_timer) {
    // send data
    collect_buffer[values_collected][0] = heart_val;
    collect_buffer[values_collected][1] = read_time - collect_timer;
    values_collected++;
    enable = true;
  }
  else {
    collect_timer_on = false;
    if (last_collect_state) {
      int id = random(LONG_MAX);
      // this blocks until data sent
      post_data(values_collected, id);
    }
    enable = false;
    values_collected = 0;
  }
  last_collect_state = collect_timer_on;


  // debug to calculate # samples
  // if (values_collected > 0) {
  //   Serial.println(values_collected);
  // }

  // call display
  display.display();
  display2.display();
}
