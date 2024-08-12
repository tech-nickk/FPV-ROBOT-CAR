#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" 
#include "soc/rtc_cntl_reg.h"  
#include "esp_http_server.h"
#include "webpage.h"  // Include the header file

const char* ssid = "robotcar";
const char* password = "robotcar";

extern String WiFiAddr = "";
WiFiServer server(81);


// PINS FOR THE MOTOR DRIVER
extern int enA = D0;
extern int in1 = D1;
extern int in2 = D2;
extern int in3 = D3;
extern int in4 = D4;
extern int enB = D5;

extern int led = D6;
extern int GND = D7;

int state = -1;
enum {
  STATE_STOP  = -1,
  STATE_FRONT = 0,
  STATE_BACK  = 1,
  STATE_RIGHT = 2,
  STATE_LEFT  = 3
};



void moveCar(int state);

#define PART_BOUNDARY "123456789000000000000987654321"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}



static esp_err_t index_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    String page = WEBPAGE;

 
    return httpd_resp_send(req, &page[0], strlen(&page[0]));
}

static esp_err_t go_handler(httpd_req_t *req){
    state = 0;
    moveCar(state);
    Serial.println("Go");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}
static esp_err_t back_handler(httpd_req_t *req){
    state = 1;
    moveCar(state);
    Serial.println("Back");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}


static esp_err_t right_handler(httpd_req_t *req){
    state = 2;
    moveCar(state);
    Serial.println("Right");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t left_handler(httpd_req_t *req){
    state = 3;
    moveCar(state);
    Serial.println("Left");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t stop_handler(httpd_req_t *req){
    state = -1;
    moveCar(state);
    Serial.println("Stop");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t ledon_handler(httpd_req_t *req){
    digitalWrite(GND, LOW);
    Serial.println("LED ON");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}
static esp_err_t ledoff_handler(httpd_req_t *req){
    digitalWrite(GND, HIGH);
    Serial.println("LED OFF");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "OK", 2);
}


void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t go_uri = {
        .uri       = "/go",
        .method    = HTTP_GET,
        .handler   = go_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t back_uri = {
        .uri       = "/back",
        .method    = HTTP_GET,
        .handler   = back_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stop_uri = {
        .uri       = "/stop",
        .method    = HTTP_GET,
        .handler   = stop_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t left_uri = {
        .uri       = "/left",
        .method    = HTTP_GET,
        .handler   = left_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t right_uri = {
        .uri       = "/right",
        .method    = HTTP_GET,
        .handler   = right_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t ledon_uri = {
        .uri       = "/ledon",
        .method    = HTTP_GET,
        .handler   = ledon_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t ledoff_uri = {
        .uri       = "/ledoff",
        .method    = HTTP_GET,
        .handler   = ledoff_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    // httpd_uri_t status_uri = {
    //     .uri       = "/status",
    //     .method    = HTTP_GET,
    //     .handler   = status_handler,
    //     .user_ctx  = NULL
    // };

    // httpd_uri_t cmd_uri = {
    //     .uri       = "/control",
    //     .method    = HTTP_GET,
    //     .handler   = cmd_handler,
    //     .user_ctx  = NULL
    // };

    // httpd_uri_t capture_uri = {
    //     .uri       = "/capture",
    //     .method    = HTTP_GET,
    //     .handler   = capture_handler,
    //     .user_ctx  = NULL
    // };

   httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };


   //ra_filter_init(&ra_filter, 20);
    Serial.printf("Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &go_uri); 
        httpd_register_uri_handler(stream_httpd, &back_uri); 
        httpd_register_uri_handler(stream_httpd, &stop_uri); 
        httpd_register_uri_handler(stream_httpd, &left_uri);
        httpd_register_uri_handler(stream_httpd, &right_uri);
        httpd_register_uri_handler(stream_httpd, &ledon_uri);
        httpd_register_uri_handler(stream_httpd, &ledoff_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  Serial.begin(115200);
  //Serial.setDebugOutput(false);

    pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);


  pinMode(led, OUTPUT);
  pinMode(GND, OUTPUT);


  digitalWrite(led, LOW);
  digitalWrite(GND, HIGH);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: http://");
  Serial.println(IP);

  server.begin();
  
  // Start streaming web server
  startCameraServer();

    WiFiAddr = IP.toString();
}

void loop() {
  delay(1);
}


void moveCar(int state)
{
    Serial.println("- Characteristic state has changed!");
  
   switch (state) {
      case STATE_FRONT:
      
      
        Serial.println("* Actual value: FRONT ");
        Serial.println(" ");

        analogWrite(enA, 255);
        analogWrite(enB, 255);
        
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        
        digitalWrite(in3, HIGH);
        digitalWrite(in4, LOW);
        
        
        break;
      case STATE_BACK:
        Serial.println("* Actual value: BACK");
        Serial.println(" ");

        
        analogWrite(enA, 255);
        analogWrite(enB, 255);
        
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        
        digitalWrite(in3, LOW);
        digitalWrite(in4, HIGH);
        
        break;
      case STATE_RIGHT:
        Serial.println("* Actual value: LEFT ");
        Serial.println(" ");

        analogWrite(enA, 255);
        analogWrite(enB, 255);
        
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        
        digitalWrite(in3, HIGH);
        digitalWrite(in4, LOW);
        
        break;
      case STATE_LEFT:
        Serial.println("* Actual value: RIGHT");
        Serial.println(" ");

        
        analogWrite(enA, 225);
        analogWrite(enB, 225);
        
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        
        digitalWrite(in3, LOW);
        digitalWrite(in4, HIGH);
        
        break;

      case STATE_STOP:
        Serial.println("* Actual value: STOP");
        Serial.println(" ");

        
        analogWrite(enA, 0);
        analogWrite(enB, 0);
        
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        
        digitalWrite(in3, LOW);
        digitalWrite(in4, LOW);
        
        break;

      default:
      
        analogWrite(enA, 0);
        analogWrite(enB, 0);
        
        break;
    }  
}

