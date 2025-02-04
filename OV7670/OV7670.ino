#include <SPI.h>
#include <Arduino.h>

#include "I2C.h"
#include "FifoCamera.h"

#include <WiFi.h>
#include <HTTPClient.h>

/* LCD includes start */
#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"
/* LCD includes end */

const int VSYNC = 32; //vertical sync
const int SIOD = 21; //SDA
const int SIOC = 22; //SCL

const int RRST = 17;  //read reset
const int WRST = 12;  //write reset
const int RCK = 4;    //read clock
const int WR = 5;     //write flag
//OE -> GND     (output enable always on since we control the read clock)
//PWDN not nonnected  
//HREF not connected
//STR not connected
//RST -> 3.3V 

const int D0 = 13;
const int D1 = 33;
const int D2 = 14;
const int D3 = 27;
const int D4 = 26;
const int D5 = 25;
const int D6 = 35;
const int D7 = 34;

I2C<SIOD, SIOC> i2c;
FifoCamera<I2C<SIOD, SIOC>, RRST, WRST, RCK, WR, D0, D1, D2, D3, D4, D5, D6, D7> camera(i2c);

#define QQVGA

#ifdef QQVGA
const int XRES = 160;
const int YRES = 120;
#endif
#ifdef QQQVGA
const int XRES = 80;
const int YRES = 60;
#endif

const int BYTES_PER_PIXEL = 2;
const int frameSize = XRES * YRES * BYTES_PER_PIXEL;
unsigned char frame[frameSize];

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* server_url = "http://192.168.116.53:5000/recognize";

TaskHandle_t TaskGetFrame;
TaskHandle_t TaskSendFrame;
TaskHandle_t TaskDisplayLCD;
SemaphoreHandle_t mutex;

bool frame_sent = true;
bool frame_shown = true;


void init_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

/* Core 0 task */
void send_frame(void *param) {
  while (1) {
    if (frame_sent) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    HTTPClient http;
    http.begin(server_url);
    http.addHeader("Content-Type", "application/octet-stream");

    int httpResponseCode = -1;
    httpResponseCode = http.POST(frame, frameSize);
    frame_sent = true;
    
    if (httpResponseCode > 0) {
      Serial.println("Frame sent successfully!");
    } else {
      Serial.println("Error sending frame.");
    }

    http.end();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/* Core 1 task */
/*
If you take semaphore only on .readFrame() you get image split in 4
Reason is unknown to me
*/
void get_frame(void *param) {
  while (1) {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {  // Take the mutex
      while(!digitalRead(VSYNC));
      while(digitalRead(VSYNC));
      camera.prepareCapture();
      camera.startCapture();
      while(!digitalRead(VSYNC));
      camera.stopCapture();

      camera.readFrame(frame, XRES, YRES, BYTES_PER_PIXEL);
      frame_sent = false;
      frame_shown = false;
      
      xSemaphoreGive(mutex);  // Release the mutex
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initialization...");
  i2c.init();
  camera.init();
  
  #ifdef QQVGA
    camera.QQVGARGB565();
  #endif
  #ifdef QQQVGA
    camera.QQQVGARGB565();
  #endif
  
  
  pinMode(VSYNC, INPUT);

  /* LCD setup start */
  Config_Init();
  LCD_Init();
  LCD_Clear(0xffff);
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
  Paint_Clear(WHITE);
  /* LCD setup end */

  /* WiFi setup start */
  init_wifi();
  /* WiFi setup end */

  /* RTOS setup start */
  mutex = xSemaphoreCreateMutex();
  if (mutex == NULL) {
    Serial.println("Mutex wasn't created");
    ESP.restart();
  }

  xTaskCreatePinnedToCore(
      get_frame, /* Function to implement the task */
      "GetCamFrame", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &TaskGetFrame,  /* Task handle. */
      0); /* Core where the task should run */

  xTaskCreatePinnedToCore(
      send_frame, /* Function to implement the task */
      "WiFi_transmit", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &TaskSendFrame,  /* Task handle. */
      0); /* Core where the task should run */

  xTaskCreatePinnedToCore(
      displayRGB565, /* Function to implement the task */
      "DisplayLCD", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &TaskDisplayLCD,  /* Task handle. */
      1); /* Core where the task should run */
  /* RTOS setup end */
}

void displayRGB565(void *param)
{
  while (1) {
    if (frame_shown) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {  // Take the mutex
      int i = 0;
      for(int x = 0; x < XRES; x++) {
        for(int y = 0; y < YRES; y++) {
          i = (y * XRES + x) << 1;
          Paint_SetPixel(x, y, frame[i] | (frame[i + 1] << 8));
        }
      }
      frame_shown = true;
      xSemaphoreGive(mutex);  // Release the mutex
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void testTFT() // green color test
{
  int i = 0;
  for(int y = 0; y < XRES; y++)
    for(int x = 0; x < YRES; x++)
      Paint_SetPixel(x, y, 0x7E0);
}

void frameToSerial()
{
  int i = 0;
  Serial.println("var frame=[");
  for(int y = 0; y < YRES; y+=1)
  {
    i = y * XRES * 2;
    for(int x = 0; x < XRES * 2; x+=1)
    {
      //Serial.print(frame[i + x], HEX);
      Serial.print(frame[i + x]);
      Serial.print(',');
    }
    Serial.println();
  }
  Serial.println("];");  
}

void loop() 
{
  vTaskDelay(pdMS_TO_TICKS(1));
}

