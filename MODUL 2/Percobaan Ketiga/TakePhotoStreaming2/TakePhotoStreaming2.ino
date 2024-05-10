/* ======================================== Including the libraries */
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "FS.h"      //--> SD Card ESP32
#include "SD_MMC.h"  //--> SD Card ESP32
#include "SPI.h"
#include "fb_gfx.h"
#include "soc/soc.h"           //--> disable brownout problems
#include "soc/rtc_cntl_reg.h"  //--> disable brownout problems
#include "esp_http_server.h"
/* ======================================== */

/* ======================================== Select camera model */
// This project was tested with the AI Thinker Model, M5STACK PSRAM Model and M5STACK WITHOUT PSRAM
#define CAMERA_MODEL_AI_THINKER
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM

// Not tested with this model
//#define CAMERA_MODEL_WROVER_KIT
/* ======================================== */

/* ======================================== GPIO of camera models */
#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM 27
#define SIOD_GPIO_NUM 25
#define SIOC_GPIO_NUM 23

#define Y9_GPIO_NUM 19
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 5
#define Y4_GPIO_NUM 34
#define Y3_GPIO_NUM 35
#define Y2_GPIO_NUM 32
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM 26
#define PCLK_GPIO_NUM 21

#elif defined(CAMERA_MODEL_M5STACK_WITHOUT_PSRAM)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM 27
#define SIOD_GPIO_NUM 25
#define SIOC_GPIO_NUM 23

#define Y9_GPIO_NUM 19
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 5
#define Y4_GPIO_NUM 34
#define Y3_GPIO_NUM 35
#define Y2_GPIO_NUM 17
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM 26
#define PCLK_GPIO_NUM 21

#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Camera model not selected"
#endif
/* ======================================== */

/* ======================================== Replace with your network credentials */
const char *ssid = "***********";
const char *password = "***********";
/* ======================================== */

int tpic = 2;  //--> variable for the process status of saving images to MicroSD card.

/* ======================================== variable to accommodate the results of reading text files from the MicroSD card. */
/*
 * char txt_Text[10]; --> this variable to accommodate the results of reading text files from the MicroSD card
 * in the text file "IFNO.txt" will be written 1 to 999999999.
 * So the image file name is from IMG_000000001 to IMG_999999999.
 * So if the image file name has reached IMG_999999999, please move all image files to another storage, because after IMG_999999999 it will repeat to IMG_000000001.
 */
char txt_Text[10];
int txt_Text_Count = 0;
/* ======================================== */

/* ======================================== */
typedef struct {
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;
/* ======================================== */

/* ======================================== */
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
/* ======================================== */

/* ======================================== Empty handle to esp_http_server */
httpd_handle_t index_httpd = NULL;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
httpd_handle_t capture_httpd = NULL;
/* ======================================== */

/* ======================================== HTML code for index / main page */
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
  <head>
    <title>ESP32-CAM Stream Web Server with Take Picture and Save to MicroSD Card</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;}
      /* ----------------------------------- Toggle Switch */
     .switch {
        position: relative;
        display: inline-block;
        width: 90px;
        height: 34px;
        top: 10px;
      }
     .switch input {display:none;}
     .slider {
        position: absolute;
        cursor: pointer;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: #D3D3D3;
        -webkit-transition:.4s;
        transition:.4s;
        border-radius: 34px;
      }
     .slider:before {
        position: absolute;
        content: "";
        height: 26px;
        width: 26px;
        left: 4px;
        bottom: 4px;
        background-color: #f7f7f7;
        -webkit-transition:.4s;
        transition:.4s;
        border-radius: 50%;
      }
      input:checked +.slider {
        background-color: #5b9dd8;
      }
      input:focus +.slider {
        box-shadow: 0 0 1px #2196F3;
      }
      input:checked +.slider:before {
        -webkit-transform: translateX(26px);
        -ms-transform: translateX(26px);
        transform: translateX(55px);
      }
     .slider:after {
        content:'OFF';
        color: white;
        display: block;
        position: absolute;
        transform: translate(-50%,-50%);
        top: 50%;
        left: 70%;
        font-size: 10px;
        font-family: Verdana, sans-serif;
      }
      input:checked +.slider:after {  
        left: 25%;
        content:'ON';
      }
      /* ----------------------------------- */
      /* ----------------------------------- button1 / Capture Button */
     .button1 {
        display: inline-block;
        padding: 10px 20px;
        font-size: 14px;
        cursor: pointer;
        text-align: center;
        text-decoration: none;
        outline: none;
        color: #fff;
        background-color: #4CAF50;
        border: none;
        border-radius: 30px;
        vertical-align: top; /* Add this line */
      }
     .button1:hover {background-color: #3e8e41}
     .button1:active {
        transform: scale(0.9,0.9)
      }
     .button1:disabled {
        opacity: 0.6;
        cursor: not-allowed;
        pointer-events: none;
      }
      /* ----------------------------------- */
      /* ----------------------------------- button2 / Stream Button */
     .button2 {
        display: inline-block;
        padding: 10px 20px;
        font-size: 14px;
        cursor: pointer;
        text-align: center;
        text-decoration: none;
        outline: none;
        color: #fff;
        background-color: #935cfb;
        border: none;
        border-radius: 30px;
        vertical-align: top;
      }
     .button2:hover {background-color: #7c38fa}
     .button2:active {
        transform: scale(0.9,0.9)
      }
     .button2:disabled {
        opacity: 0.6;
        cursor: not-allowed;
        pointer-events: none;
      }
      /* ----------------------------------- */
      /* ----------------------------------- button3 / Rotate Button */
     .button3 {
        display: inline-block;
        padding: 10px 20px;
        font-size: 14px;
        cursor: pointer;
        text-align: center;
        text-decoration: none;
        outline: none;
        color: #fff;
        background-color: #ff69b4;
        border: none;
        border-radius: 30px;
        vertical-align: top;
      }
     .button3:hover {background-color: #ff33cc}
     .button3:active {
        transform: scale(0.9,0.9)
      }
     .button3:disabled {
        opacity: 0.6;
        cursor: not-allowed;
        pointer-events: none;
      }
      /* ----------------------------------- */
      /* ----------------------------------- Stream and Capture Viewer */
      img {
        width: auto ;
        max-width: 70% ;
        height: auto ;
        max-height: 70% ;
        display: block;
        margin: 0 auto;
      }
      /* ----------------------------------- */
    </style>   
  </head> 
  <body> 
    <h3>ESP32-CAM Stream Web Server with Take Picture and Save to MicroSD Card</h3>  
    <img src="" id="photo">    
    <br><br>    
    <span style="font-size:15;">LED Flash : &nbsp;</span>    
    <label class="switch">
      <input type="checkbox" id="togLEDFlash" onclick="LEDFlash()">
      <div class="slider round"></div>
    </label>   
    <br><br>   
    <button class="button1" onclick="viewer('capture')" id="Cptr">Capture</button>
    <button class="button2" onclick="viewer('stream')" id="Strm">Stream</button>
    <button class="button3" onclick="rotateImage()" id="Rt">Rotate</button>
    <br><br>   
    <p id="st"></p>    
    <script>
      /* ----------------------------------- Calls the video stream link and displays it */ 
      window.onload = document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";
      document.getElementById("Cptr").disabled = false;
      document.getElementById("Strm").disabled = true;
      send_cmd("stream");
      /* ----------------------------------- */
      /* ----------------------------------- Information text to be displayed when saving image to MicroSD card */
      var myTmr;
      var itext = 31;
      let text = "Saving pictures to MicroSD card...";
      var valTaTephotostate = 10;
      /* ----------------------------------- */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Function to send commands to turn on or turn off the LED Flash */
      function LEDFlash() {
        var tgLEDFlash = document.getElementById("togLEDFlash");
        var tgState;
        if (tgLEDFlash.checked == true) tgState = 1;
        if (tgLEDFlash.checked == false) tgState = 0;
        send_cmd(tgState);
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Function to choose to show streaming video or captured image */
      function viewer(x) {
        if (x == "capture") {
          document.getElementById("Cptr").disabled = true;
          send_cmd(x);
          window.stop();
          document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/capture";
          textSaveToMicroSD();
          start_timer();
        } 
        if (x == "stream") {
          window.location.reload();
        }
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Function for sending commands */
      function send_cmd(cmds) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/action?go=" + cmds, true);
        xhr.send();
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Start and stop the timer */
      function start_timer() {
        myTmr = setInterval(myTimer, 1000)
      }     
      function stop_timer() {
        clearInterval(myTmr)
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Timer to check image saving process to MicroSD card */
      function myTimer() {
        getTakephotostate();
        textSaveToMicroSD();
        if (valTaTephotostate == 0) {
          document.getElementById("st").innerHTML = "";
          document.getElementById("Strm").disabled = false;
          stop_timer();
        }
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Function to display text information when saving image to MicroSD card */
      function textSaveToMicroSD() {
        itext++;
        if(itext>34) itext = 31;
        let result = text.substring(0, itext);
        document.getElementById("st").innerHTML = result;
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Function to receive the process status of saving images to MicroSD card */
      function getTakephotostate() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            valTaTephotostate = this.responseText;
          }
        };
        xhttp.open("GET", "/takephotostate", true);
        xhttp.send();
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
      /* :::::::::::::::::::::::::::::::::::::::::::::::: Function to rotate the image */
      var rotateAngle = 0;
      function rotateImage() {
        rotateAngle += 90;
        document.getElementById("photo").style.transform = "rotate(" + rotateAngle + "deg)";
        send_cmd("rotate");
      }
      /* :::::::::::::::::::::::::::::::::::::::::::::::: */
    </script>
  </body>
</html>
)rawliteral";
/* ======================================== */

/* ________________________________________________________________________________ Index handler function to be called during GET or uri request */
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ jpg_encode_stream handler function to be called during GET or uri request */
static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (!index) {
    j->len = 0;
  }
  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
    return 0;
  }
  j->len += len;
  return len;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ capture handler function to be called during GET or uri request */
static esp_err_t capture_handler(httpd_req_t *req) {
  tpic = 1;
  bool ledflashLastState = false;
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  int64_t fr_start = esp_timer_get_time();

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  /* Save image to microSD card */
  char pathIMG[20];
  sprintf(pathIMG, "/IMG_%09d.jpg", millis());  // use millis() as a unique filename
  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", pathIMG);

  File file = fs.open(pathIMG, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len);  // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", pathIMG);
  }
  file.close();

  /*... */

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  size_t out_len, out_width, out_height;
  uint8_t *out_buf;
  bool s;
  bool detected = false;
  int face_id = 0;
  if (true) {
    size_t fb_len = 0;
    if (fb->format == PIXFORMAT_JPEG) {
      fb_len = fb->len;
      res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    } else {
      jpg_chunking_t jchunk = { req, 0 };
      res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
      httpd_resp_send_chunk(req, NULL, 0);
      fb_len = jchunk.len;
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
    tpic = 0;
    return res;
  }
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ stream handler function to be called during GET or uri request. */
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if (fb->width > 400) {
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
    if (res != ESP_OK) {
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ cmd handler function to be called during GET or uri request. */
static esp_err_t cmd_handler(httpd_req_t *req) {
  char *buf;
  size_t buf_len;
  char variable[32] = {
    0,
  };

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char *)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int res = 0;

  if (!strcmp(variable, "1")) {
    digitalWrite(4, HIGH);
    Serial.println("LED Flash ON");
  } else if (!strcmp(variable, "0")) {
    digitalWrite(4, LOW);
    Serial.println("LED Flash OFF");
  } else if (!strcmp(variable, "capture")) {
    Serial.println("capture");
  } else if (!strcmp(variable, "stream")) {
    Serial.println("stream");
  } else {
    res = -1;
  }

  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ takephotostate handler function to be called during GET or uri request. */
static esp_err_t takephotostate_handler(httpd_req_t *req) {
  char tpic_buf[12];
  httpd_resp_send(req, itoa(tpic, tpic_buf, 10), HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for starting the web server / startCameraServer. */
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
  };

  httpd_uri_t cmd_uri = {
    .uri = "/action",
    .method = HTTP_GET,
    .handler = cmd_handler,
    .user_ctx = NULL
  };

  httpd_uri_t takephotostate_uri = {
    .uri = "/takephotostate",
    .method = HTTP_GET,
    .handler = takephotostate_handler,
    .user_ctx = NULL
  };

  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
  };

  httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
  };

  if (httpd_start(&index_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(index_httpd, &index_uri);
    httpd_register_uri_handler(index_httpd, &cmd_uri);
    httpd_register_uri_handler(index_httpd, &takephotostate_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &capture_uri);
  }

  Serial.println();
  Serial.println("Camera Server started successfully");
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());
  Serial.println();
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine to read text in a text file on a MicroSD card. */
void readFile(fs::FS &fs, const char *path) {
  Serial.println();
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  memset(txt_Text, 0, sizeof txt_Text);

  Serial.print("Read from file: ");
  while (file.available()) {
    //Serial.write(file.read());
    char inChar = file.read();
    txt_Text[txt_Text_Count] = inChar;
    txt_Text_Count++;
  }
  file.close();
  Serial.println(txt_Text);
  Serial.println();
  txt_Text_Count = 0;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for writing text to a text file on the MicroSD card. */
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.println();
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
  Serial.println();
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID SETUP() */
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //--> disable brownout detector

  Serial.begin(115200);
  Serial.println();
  delay(1000);
  Serial.setDebugOutput(false);

  /* ---------------------------------------- Camera configuration. */
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;  //UXGA;
    /*
     * From the results of the tests I did:
     * - config.jpeg_quality = 10; --> the captured images are good for indoors or in low light conditions.
     *   But the image file is "corrupted" for capturing images outdoors or in bright light conditions.
     * - config.jpeg_quality = 20; --> there is no "corrupt" of image files for image capture both indoors and outdoors.
     * 
     * I don't know if this only happens to my ESP32 Cam module. Please test the settings above.
     * 
     * From source: https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/ , The image quality (jpeg_quality) can be a number between 0 and 63.
     * A lower number means a higher quality. However, very low numbers for image quality,
     * specially at higher resolution can make the ESP32-CAM to crash or it may not be able to take the photos properly. 
     */
    config.jpeg_quality = 20;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Camera init. */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Start accessing and checking SD card. */
  /*
   * The "SD_MMC.h" library supports 4-bit and 1-bit modes for accessing MicroSD cards.
   * 
   * The 4-bit mode uses GPIO 2, 4, 12, 13, 14, and 15. That means the LED Flash on Borad on the ESP32 Cam cannot be used, because the LED Flash uses GPIO_4.
   * 
   * The 1-bit mode does not use GPIO 4, 12 and 13 (based on some references I read).
   * So GPIO 4, 12 and 13 can be used for other purposes.
   * But in the tests that I tried on the ESP32 Cam that I have (ESP32-CAM AI-Thinker), it seems that GPIO 4, 12 and 13 are still being used.
   * However, if you use 1-bit mode, GPIO 4, 12 and 13 can still be used for other purposes with a few programming tricks.
   * Unlike the 4-bit mode, where GPIO 4, 12 and 13 cannot be used for other purposes at all.
   * 
   * I CONFIRM THE ABOVE DESCRIPTION IS BASED ON TESTING I HAVE DONE ON THE ESP32 CAM I HAVE.
   * I might be wrong. Please leave a comment about it in the comments section of this video project (on Youtube).
   * 
   * So in this project I'm using 1-bit mode.
   */

  /* ::::::: 4-bit mode */
  //  Serial.println("Start accessing SD Card 4-bit mode");
  //  if(!SD_MMC.begin()){
  //    Serial.println("SD Card Mount Failed");
  //    return;
  //  }
  //  Serial.println("Started accessing SD Card 4-bit mode successfully");
  /* ::::::: */

  /* ::::::: 1-bit mode */
  Serial.println("Starting SD Card");
  pinMode(13, INPUT_PULLUP);  //--> This is done to resolve an "error" in 1-bit mode when SD_MMC.begin("/sdcard", true). Reference: https://github.com/espressif/arduino-esp32/issues/4680

  Serial.println("Start accessing SD Card 1-bit mode");
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("Started accessing SD Card 1-bit mode successfully");

  pinMode(13, INPUT_PULLDOWN);
  /* ::::::: */
  /* ---------------------------------------- */

  /* ---------------------------------------- Checking SD card type */
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }
  /* ---------------------------------------- */

  delay(1000);

  pinMode(4, OUTPUT);  //--> LED Flash on Board

  /* ---------------------------------------- Wi-Fi connection */
  Serial.println();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(4, HIGH);
    delay(250);
    digitalWrite(4, LOW);
    delay(250);
    Serial.print(".");
  }
  /* ---------------------------------------- */

  digitalWrite(4, LOW);
  delay(1000);
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();  //--> Start camera web server
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID LOOP() */
void loop() {
  delay(1);
}