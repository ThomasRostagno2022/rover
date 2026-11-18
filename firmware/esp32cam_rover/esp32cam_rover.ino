/*
  ESP32-CAM Rover firmware  (Option B)
  ------------------------------------
  Board: "AI Thinker ESP32-CAM"  (Arduino IDE -> Tools -> Board)

  What it does:
    * Joins your WiFi.
    * Runs a tiny web server:
        /                       -> a page with a live photo + drive buttons
        /capture                -> one JPEG photo
        /drive?l=1&r=1&ms=500   -> left/right wheels (-1, 0, 1) for ms milliseconds
        /stop                   -> stop motors
        /lights?on=1            -> built-in flash LED on as a headlight (on=0 = off)
    * Your laptop runs the Python "brain" (python -m rover mission --hardware esp32 ...),
      which grabs /capture, asks Claude what to do, then calls /drive.

  SAFETY: every /drive call stops the motors by itself after `ms` (max 3000).
  The rover can never drive off forever, even if WiFi drops.

  Wiring (see docs/03-build-option-b-esp32.md):
      ESP32-CAM GPIO14 -> TB6612 AIN1   (left motor)
      ESP32-CAM GPIO15 -> TB6612 AIN2
      ESP32-CAM GPIO13 -> TB6612 BIN1   (right motor)
      ESP32-CAM GPIO12 -> TB6612 BIN2
      TB6612 PWMA, PWMB, STBY, VCC -> ESP32-CAM 3.3V
      TB6612 GND -> ESP32-CAM GND -> battery pack (-)
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ======= CHANGE THESE TO YOUR WIFI (2.4 GHz network - ESP32 can't do 5 GHz) =======
const char* WIFI_NAME     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
// ====================================================================================

// Motor pins
const int LEFT_IN1  = 14;
const int LEFT_IN2  = 15;
const int RIGHT_IN1 = 13;
const int RIGHT_IN2 = 12;

// The bright white "flash" LED built into the ESP32-CAM board, used as a headlight.
const int HEADLIGHT_PIN = 4;

// If a side drives backwards when it should go forward, change its 1 to -1.
const int LEFT_DIRECTION  = 1;
const int RIGHT_DIRECTION = 1;

// Camera pins for the AI Thinker ESP32-CAM board
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

WebServer server(80);

// ---------------------------------------------------------------- motors
void setWheel(int in1, int in2, int dir) {
  // dir: 1 = forward, -1 = backward, 0 = stop
  digitalWrite(in1, dir > 0 ? HIGH : LOW);
  digitalWrite(in2, dir < 0 ? HIGH : LOW);
}

void setMotors(int left, int right) {
  setWheel(LEFT_IN1, LEFT_IN2, left * LEFT_DIRECTION);
  setWheel(RIGHT_IN1, RIGHT_IN2, right * RIGHT_DIRECTION);
}

void stopMotors() { setMotors(0, 0); }

// ---------------------------------------------------------------- camera
bool startCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;   // 640x480: enough for Claude, small to send
  config.jpeg_quality = 12;              // 10-63, lower = better quality
  if (psramFound()) {
    config.fb_count    = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode   = CAMERA_GRAB_LATEST;  // always return the newest frame
  } else {
    config.fb_count    = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  // If the picture is upside down, uncomment these two lines:
  // sensor_t* s = esp_camera_sensor_get();
  // s->set_vflip(s, 1); s->set_hmirror(s, 1);
  return true;
}

// ---------------------------------------------------------------- web pages
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rover</title><style>
body{font-family:sans-serif;text-align:center;background:#111;color:#eee}
img{width:100%;max-width:640px;border-radius:8px}
button{font-size:1.4em;width:4.5em;height:2.5em;margin:.2em;border-radius:8px}
</style></head><body>
<h2>Rover camera</h2>
<img id="cam" src="/capture"><br>
<div><button onclick="go(1,1)">&#8593;</button></div>
<div><button onclick="go(-1,1)">&#8634;</button><button onclick="fetch('/stop')">stop</button><button onclick="go(1,-1)">&#8635;</button></div>
<div><button onclick="go(-1,-1)">&#8595;</button></div>
<div><button onclick="light()">&#128161;</button></div>
<script>
function refresh(){document.getElementById('cam').src='/capture?t='+Date.now();}
function go(l,r){fetch('/drive?l='+l+'&r='+r+'&ms=400').then(refresh);}
var lit=0;
function light(){lit=1-lit;fetch('/lights?on='+lit).then(refresh);}
</script></body></html>
)HTML";

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { server.send(500, "text/plain", "camera capture failed"); return; }
  if (!psramFound()) {
    // With a single buffer the first frame may be old: throw it away.
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();
    if (!fb) { server.send(500, "text/plain", "camera capture failed"); return; }
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleDrive() {
  int l  = constrain(server.arg("l").toInt(), -1, 1);
  int r  = constrain(server.arg("r").toInt(), -1, 1);
  int ms = constrain(server.arg("ms").toInt(), 0, 3000);
  Serial.printf("drive l=%d r=%d ms=%d\n", l, r, ms);
  setMotors(l, r);
  delay(ms);          // drive...
  stopMotors();       // ...then ALWAYS stop
  server.send(200, "text/plain", "ok");
}

void handleLights() {
  bool on = server.arg("on").toInt() == 1;
  digitalWrite(HEADLIGHT_PIN, on ? HIGH : LOW);
  Serial.printf("headlights %s\n", on ? "on" : "off");
  server.send(200, "text/plain", on ? "lights on" : "lights off");
}

void handleStop() {
  stopMotors();
  server.send(200, "text/plain", "stopped");
}

// ---------------------------------------------------------------- setup / loop
void setup() {
  Serial.begin(115200);
  Serial.println("\nRover booting...");

  pinMode(LEFT_IN1, OUTPUT);  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT); pinMode(RIGHT_IN2, OUTPUT);
  stopMotors();

  pinMode(HEADLIGHT_PIN, OUTPUT);
  digitalWrite(HEADLIGHT_PIN, LOW);  // headlight off at start

  if (!startCamera()) {
    Serial.println("Check the camera ribbon cable, then press RST.");
    while (true) delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // faster replies
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Rover ready! Open this in your browser:  http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/drive", handleDrive);
  server.on("/stop", handleStop);
  server.on("/lights", handleLights);
  server.begin();
}

void loop() {
  server.handleClient();
}
