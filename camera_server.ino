#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// =============================================================
// 1. CONFIGURATION
// =============================================================

// WiFi Access Point Credentials
const char* ssid = "ESP32-CAM-Stream"; //Change with your desired AP ssid
const char* password = "password123"; ////Change with your desired AP password

// LED Flash Pin (GPIO 4 on AI Thinker)
#define LED_GPIO_NUM 4

// Camera Pin Definition (AI Thinker Model)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// =============================================================
// 2. VIDEO STREAM MACROS
// =============================================================
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

// =============================================================
// 3. WEB PAGE HTML (Full Screen + Slider)
// =============================================================
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { 
        margin: 0; 
        padding: 0; 
        background-color: #000; 
        overflow: hidden; /* Hide scrollbars */
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        height: 100vh;
        width: 100vw;
      }
      
      /* Video Container */
      #stream-container {
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        z-index: 1;
      }
      
      img { 
        width: 100%; 
        height: 100%; 
        object-fit: fill; /* Forces image to stretch to fill screen */
        display: block;
      }

      /* Slider Container */
      .slider-container {
        position: absolute;
        bottom: 20px;
        z-index: 2; /* Sit on top of video */
        width: 60%;
        background: rgba(0, 0, 0, 0.5);
        padding: 10px;
        border-radius: 20px;
        display: flex;
        justify-content: center;
      }

      /* Slider Style */
      input[type=range] {
        width: 100%;
        -webkit-appearance: none;
        background: transparent;
      }
      input[type=range]::-webkit-slider-thumb {
        -webkit-appearance: none;
        height: 20px;
        width: 20px;
        border-radius: 50%;
        background: #ffffff;
        cursor: pointer;
        margin-top: -8px;
      }
      input[type=range]::-webkit-slider-runnable-track {
        width: 100%;
        height: 4px;
        cursor: pointer;
        background: #ddd;
        border-radius: 2px;
      }
    </style>
  </head>
  <body>
    <div id="stream-container">
      <img src="/stream" id="stream">
    </div>

    <div class="slider-container">
      <input type="range" min="0" max="255" value="0" id="ledSlider" oninput="updateLed(this.value)">
    </div>

    <script>
      function updateLed(val) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/led?val=" + val, true);
        xhr.send();
      }
    </script>
  </body>
</html>
)rawliteral";

// =============================================================
// 4. HANDLERS
// =============================================================

// Handler for LED Brightness
static esp_err_t led_handler(httpd_req_t *req) {
  char* buf;
  size_t buf_len;
  char param[32];
  
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "val", param, sizeof(param)) == ESP_OK) {
        int val = atoi(param);
        // Use LEDC (PWM) to control brightness
        ledcWrite(7, val); 
      }
    }
    free(buf);
  }
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if (fb->format != PIXFORMAT_JPEG) {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if (!jpeg_converted) {
          Serial.println("JPEG compression failed");
          res = ESP_FAIL;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) break;
  }
  return res;
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t led_uri = {
    .uri       = "/led",
    .method    = HTTP_GET,
    .handler   = led_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &led_uri);
  }
}

// =============================================================
// 5. SETUP & LOOP
// =============================================================

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Setup LED PWM (Channel 7, 5000Hz, 8-bit resolution)
  ledcSetup(7, 5000, 8);
  ledcAttachPin(LED_GPIO_NUM, 7);
  ledcWrite(7, 0); // Start with LED off

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
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera Init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // FLIP AND ROTATE SETTINGS
  // Adjust these if the image is upside down!
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);   // 1 = Flip vertical
  s->set_hmirror(s, 0); // 0 = Normal horizontal
  
  // Start Access Point
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  startCameraServer();
  Serial.println("Camera Ready! Connect to 'ESP32-CAM-Stream' and go to http://192.168.4.1");
}

void loop() {
  delay(10000);
}
