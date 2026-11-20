// Mini Nod Lamp firmware (ESP32 + 2 servos + RGBW LEDs)
// -----------------------------------------------------
// The lamp's "reflexes": smooth light fades, smooth head moves, gestures,
// a daily sunrise, a touch pad and an optional motion sensor.
// It also serves a tiny web API so the Python brain (with Claude) can drive it.
//
// Arduino IDE 2 setup:
//   1. Boards Manager: install "esp32" by Espressif. Board: "ESP32 Dev Module".
//   2. Library Manager: install "Adafruit NeoPixel" and "ESP32Servo".
//   3. Fill in WIFI_SSID / WIFI_PASSWORD below, upload, then open the Serial
//      Monitor at 115200 baud to see the lamp's IP address.
//
// Web API (try them in your browser: http://<lamp-ip>/state):
//   /state                                  -> JSON with light, head, time
//   /light?kelvin=3000&brightness=40&fade_ms=2000
//   /color?r=255&g=80&b=0&brightness=30&fade_ms=1000
//   /off?fade_ms=3000
//   /pose?pan=0&tilt=-20&speed=60           (pan -80..80, tilt -40..60, deg/s)
//   /gesture?name=nod                       (see GESTURES below)
//   /sunrise?minutes=1                      (start a sunrise right now)
//   /alarm?hour=7&minute=15&minutes=30      (daily sunrise; hour=-1 disables)
//   /timer?minutes=20                       (fade out after N minutes; 0 cancels)

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

// ---------------- YOUR SETTINGS ----------------
const char* WIFI_SSID     = "your-wifi-name";
const char* WIFI_PASSWORD = "your-wifi-password";
const char* TIMEZONE      = "EST5EDT,M3.2.0,M11.1.0";   // New York

const int LED_PIN   = 16;
const int LED_COUNT = 8;      // how many LEDs you cut from the strip
const bool LED_RGBW = true;   // false for plain RGB LEDs (WS2812B / NeoPixel stick or ring)
const int PAN_PIN   = 18;
const int TILT_PIN  = 19;
const int TOUCH_PIN = 4;      // T0: a wire to copper tape or foil
const int PIR_PIN   = 34;     // optional motion sensor; set USE_PIR false if none
const bool USE_PIR  = false;

// Servo angles for "pan 0 / tilt 0" (head facing forward, level).
// Adjust after assembly so the lamp looks straight ahead at 0/0.
const int PAN_CENTER  = 90;
const int TILT_CENTER = 90;
const int TILT_SIGN   = 1;    // set to -1 if "tilt down" moves the head up
const int PAN_SIGN    = 1;    // set to -1 if left/right are swapped
const int DOOR_PAN    = 60;   // where "look at the door" points
const int READ_PAN = 0, READ_TILT = -30;   // reading pose
const int REST_PAN = 0, REST_TILT = -40;   // resting pose (light off)

// ---------------- LIGHT ----------------
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, (LED_RGBW ? NEO_GRBW : NEO_GRB) + NEO_KHZ800);
// current and target values for R, G, B, W (0..255 at full brightness)
float cur[4] = {0, 0, 0, 0}, from[4] = {0, 0, 0, 0}, to[4] = {0, 0, 0, 0};
unsigned long fadeStart = 0, fadeMs = 1;
int lightKelvin = 0, lightBrightness = 0;   // what the lamp reports in /state
String lightMode = "off";

// Map a colour temperature to the RGBW chips. The W chip is warm white
// (~3000K); add red for warmer light and blue for cooler light.
void kelvinToRGBW(int k, float out[4]) {
  k = constrain(k, 1800, 6500);
  if (k <= 3000) {
    float t = (3000.0 - k) / 1200.0;          // 0 at 3000K, 1 at 1800K
    out[0] = 255 * t;  out[1] = 70 * t;  out[2] = 0;  out[3] = 255 * (1 - 0.5 * t);
  } else {
    float t = (k - 3000.0) / 3500.0;          // 0 at 3000K, 1 at 6500K
    out[0] = 120 * t;  out[1] = 150 * t;  out[2] = 255 * t;  out[3] = 255;
  }
}

void startFade(const float target[4], int brightness, unsigned long ms) {
  float b = constrain(brightness, 0, 100) / 100.0;
  for (int i = 0; i < 4; i++) { from[i] = cur[i]; to[i] = target[i] * b; }
  fadeStart = millis(); fadeMs = max(1UL, ms);
}

void setLightKelvin(int kelvin, int brightness, unsigned long ms) {
  float t[4]; kelvinToRGBW(kelvin, t);
  startFade(t, brightness, ms);
  lightKelvin = kelvin; lightBrightness = brightness; lightMode = brightness > 0 ? "white" : "off";
}

void setLightColor(int r, int g, int b, int brightness, unsigned long ms) {
  float t[4] = {(float)r, (float)g, (float)b, 0};
  startFade(t, brightness, ms);
  lightKelvin = 0; lightBrightness = brightness; lightMode = brightness > 0 ? "color" : "off";
}

void updateLight() {
  float p = min(1.0f, (millis() - fadeStart) / (float)fadeMs);
  p = p * p * (3 - 2 * p);                      // smooth start and end
  for (int i = 0; i < 4; i++) cur[i] = from[i] + (to[i] - from[i]) * p;
  uint32_t c;
  if (LED_RGBW) c = strip.Color((uint8_t)cur[0], (uint8_t)cur[1], (uint8_t)cur[2], (uint8_t)cur[3]);
  else {  // no white chip: mix warm white from red, green and blue
    float w = cur[3];
    c = strip.Color((uint8_t)min(255.0f, cur[0] + w), (uint8_t)min(255.0f, cur[1] + w * 0.72f), (uint8_t)min(255.0f, cur[2] + w * 0.40f));
  }
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, c);
  strip.show();
}

// ---------------- HEAD (two servos) ----------------
Servo panServo, tiltServo;
float panNow = REST_PAN, tiltNow = REST_TILT, panGoal = REST_PAN, tiltGoal = REST_TILT, speedDps = 60;

void writeServos() {
  panServo.write(constrain(PAN_CENTER + PAN_SIGN * (int)round(panNow), 5, 175));
  tiltServo.write(constrain(TILT_CENTER + TILT_SIGN * (int)round(tiltNow), 5, 175));
}

void setPose(float pan, float tilt, float speed) {
  panGoal = constrain(pan, -80, 80);
  tiltGoal = constrain(tilt, -40, 60);
  speedDps = constrain(speed, 5, 240);
}

bool headMoving() { return fabs(panGoal - panNow) > 0.5 || fabs(tiltGoal - tiltNow) > 0.5; }

unsigned long lastHead = 0;
void updateHead() {
  unsigned long now = millis();
  float dt = (now - lastHead) / 1000.0; lastHead = now;
  float step = speedDps * dt;
  float dp = panGoal - panNow, dtl = tiltGoal - tiltNow;
  panNow += constrain(dp, -step, step);
  tiltNow += constrain(dtl, -step, step);
  writeServos();
}

// ---------------- GESTURES ----------------
// Each gesture is a list of keyframes, relative to where the head was when it
// started: (pan offset, tilt offset, speed deg/s, hold ms). The head returns to
// its starting pose at the end, except "droop" and "perk_up", which stay.
struct Key { float pan, tilt, speed; int holdMs; };
struct Gesture { const char* name; const Key* keys; int count; bool returnHome; };

const Key K_NOD[]     = {{0,-15,120,80},{0,5,120,60},{0,-15,120,80},{0,0,90,0}};
const Key K_SHAKE[]   = {{-18,0,140,40},{18,0,160,40},{-18,0,160,40},{18,0,160,40},{0,0,120,0}};
const Key K_DROOP[]   = {{0,-25,25,0}};
const Key K_PERK[]    = {{0,25,90,150},{0,18,60,0}};
const Key K_LEFT[]    = {{-45,5,80,700},{0,0,70,0}};
const Key K_RIGHT[]   = {{45,5,80,700},{0,0,70,0}};
const Key K_CURIOUS[] = {{12,12,60,500},{-6,6,40,300},{0,0,60,0}};
const Key K_HAPPY[]   = {{-10,10,150,0},{10,14,150,0},{-10,10,150,0},{10,14,150,0},{0,0,100,0}};
const Key K_THINK[]   = {{-20,25,40,900},{0,0,50,0}};

Gesture GESTURES[] = {
  {"nod", K_NOD, 4, true},          {"shake", K_SHAKE, 5, true},
  {"droop", K_DROOP, 1, false},     {"perk_up", K_PERK, 2, false},
  {"look_left", K_LEFT, 2, true},   {"look_right", K_RIGHT, 2, true},
  {"curious", K_CURIOUS, 3, true},  {"happy", K_HAPPY, 5, true},
  {"thinking", K_THINK, 2, true},
};
const int GESTURE_COUNT = sizeof(GESTURES) / sizeof(GESTURES[0]);

const Gesture* activeGesture = nullptr;
int gestureStep = 0; unsigned long holdUntil = 0; bool holding = false;
float gBasePan = 0, gBaseTilt = 0;

bool startGesture(const String& name) {
  if (name == "look_at_door") {   // absolute direction, then back
    static Key k[2]; k[0] = {DOOR_PAN - panGoal, 0, 70, 900}; k[1] = {0, 0, 70, 0};
    static Gesture g = {"look_at_door", k, 2, true};
    activeGesture = &g;
  } else {
    activeGesture = nullptr;
    for (int i = 0; i < GESTURE_COUNT; i++) if (name == GESTURES[i].name) activeGesture = &GESTURES[i];
    if (!activeGesture) return false;
  }
  gBasePan = panGoal; gBaseTilt = tiltGoal; gestureStep = 0; holding = false;
  const Key& k = activeGesture->keys[0];
  setPose(gBasePan + k.pan, gBaseTilt + k.tilt, k.speed);
  return true;
}

void updateGesture() {
  if (!activeGesture || headMoving()) return;
  const Key& k = activeGesture->keys[gestureStep];
  if (!holding) { holding = true; holdUntil = millis() + k.holdMs; return; }
  if (millis() < holdUntil) return;
  holding = false; gestureStep++;
  if (gestureStep >= activeGesture->count) {
    if (activeGesture->returnHome) setPose(gBasePan, gBaseTilt, 60);
    activeGesture = nullptr; return;
  }
  const Key& n = activeGesture->keys[gestureStep];
  setPose(gBasePan + n.pan, gBaseTilt + n.tilt, n.speed);
}

// ---------------- SUNRISE, ALARM, TIMER ----------------
bool sunriseOn = false; unsigned long sunriseStart = 0, sunriseMs = 0;
int alarmHour = -1, alarmMinute = 0, alarmRamp = 30, lastAlarmDay = -1;
unsigned long offAt = 0;

void startSunrise(int minutes) {
  sunriseOn = true; sunriseStart = millis(); sunriseMs = max(1, minutes) * 60000UL;
  setPose(0, 50, 30);                      // head up toward the wall
  lightMode = "sunrise";
}

void updateSunrise() {
  if (!sunriseOn) return;
  float p = min(1.0f, (millis() - sunriseStart) / (float)sunriseMs);
  // deep red -> orange -> warm white -> bright warm white
  float t[4];
  if (p < 0.35) { float q = p / 0.35; t[0] = 255; t[1] = 20 + 50 * q; t[2] = 0; t[3] = 0; }
  else if (p < 0.7) { float q = (p - 0.35) / 0.35; t[0] = 255; t[1] = 70 + 60 * q; t[2] = 0; t[3] = 200 * q; }
  else { float q = (p - 0.7) / 0.3; t[0] = 255 - 100 * q; t[1] = 130 - 30 * q; t[2] = 30 * q; t[3] = 200 + 55 * q; }
  float b = 0.02 + 0.98 * pow(p, 1.8);
  for (int i = 0; i < 4; i++) { cur[i] = t[i] * b; from[i] = to[i] = cur[i]; }
  lightBrightness = (int)(b * 100); lightKelvin = 0;
  if (p >= 1.0) { sunriseOn = false; setLightKelvin(3500, 100, 1); }
}

void checkAlarmAndTimer() {
  struct tm now;
  if (alarmHour >= 0 && getLocalTime(&now, 0)) {
    // start the ramp so it finishes at the alarm time
    int nowMin = now.tm_hour * 60 + now.tm_min;
    int startMin = alarmHour * 60 + alarmMinute - alarmRamp;
    if (startMin < 0) startMin += 1440;
    if (nowMin == startMin && lastAlarmDay != now.tm_yday) { lastAlarmDay = now.tm_yday; startSunrise(alarmRamp); }
  }
  if (offAt && millis() > offAt) { offAt = 0; sunriseOn = false; setLightKelvin(2700, 0, 8000); setPose(REST_PAN, REST_TILT, 25); }
}

// ---------------- TOUCH AND MOTION ----------------
int touchBaseline = 0; bool touchWasDown = false; unsigned long lastTouch = 0;
unsigned long nightGlowUntil = 0;

bool isNight() {
  struct tm now; if (!getLocalTime(&now, 0)) return false;
  return now.tm_hour >= 22 || now.tm_hour < 7;
}

void updateSenses() {
  int v = touchRead(TOUCH_PIN);
  bool down = v < touchBaseline * 0.7;
  if (down && !touchWasDown && millis() - lastTouch > 600) {
    lastTouch = millis(); sunriseOn = false; activeGesture = nullptr;
    if (lightBrightness > 0) { setLightKelvin(2700, 0, 1500); setPose(REST_PAN, REST_TILT, 40); }
    else { setLightKelvin(isNight() ? 2400 : 3500, isNight() ? 35 : 80, 1200); setPose(READ_PAN, READ_TILT, 50); startGesture("nod"); }
  }
  touchWasDown = down;

  if (USE_PIR && digitalRead(PIR_PIN) == HIGH && lightBrightness == 0 && isNight()) {
    setLightColor(255, 70, 0, 6, 1500);     // very dim amber
    setPose(0, -40, 30);
    nightGlowUntil = millis() + 120000;
  }
  if (nightGlowUntil && millis() > nightGlowUntil) { nightGlowUntil = 0; setLightColor(255, 70, 0, 0, 4000); }
}

// ---------------- WEB API ----------------
WebServer server(80);
int argInt(const char* name, int def) { return server.hasArg(name) ? server.arg(name).toInt() : def; }
void ok(const String& msg) { server.send(200, "application/json", "{\"ok\":true,\"done\":\"" + msg + "\"}"); }

void handleState() {
  struct tm now; char clock[20] = "unknown";
  if (getLocalTime(&now, 0)) strftime(clock, sizeof(clock), "%Y-%m-%d %H:%M", &now);
  String j = "{\"light\":{\"mode\":\"" + lightMode + "\",\"kelvin\":" + lightKelvin + ",\"brightness\":" + lightBrightness + "},";
  j += "\"head\":{\"pan\":" + String((int)panNow) + ",\"tilt\":" + String((int)tiltNow) + "},";
  j += "\"alarm\":" + (alarmHour < 0 ? String("null") : "\"" + String(alarmHour) + ":" + (alarmMinute < 10 ? "0" : "") + String(alarmMinute) + "\"") + ",";
  j += "\"off_timer_min\":" + String(offAt ? (int)((offAt - millis()) / 60000) : -1) + ",";
  j += "\"time\":\"" + String(clock) + "\"}";
  server.send(200, "application/json", j);
}

void setupServer() {
  server.on("/state", handleState);
  server.on("/light", [] { sunriseOn = false; setLightKelvin(argInt("kelvin", 3000), argInt("brightness", 50), argInt("fade_ms", 1500)); ok("light"); });
  server.on("/color", [] { sunriseOn = false; setLightColor(argInt("r", 255), argInt("g", 120), argInt("b", 40), argInt("brightness", 50), argInt("fade_ms", 1500)); ok("color"); });
  server.on("/off", [] { sunriseOn = false; setLightKelvin(2700, 0, argInt("fade_ms", 2000)); setPose(REST_PAN, REST_TILT, 30); ok("off"); });
  server.on("/pose", [] { activeGesture = nullptr; setPose(argInt("pan", 0), argInt("tilt", 0), argInt("speed", 60)); ok("pose"); });
  server.on("/gesture", [] {
    if (startGesture(server.arg("name"))) ok("gesture");
    else server.send(400, "application/json", "{\"ok\":false,\"error\":\"unknown gesture\"}");
  });
  server.on("/sunrise", [] { startSunrise(argInt("minutes", 30)); ok("sunrise"); });
  server.on("/alarm", [] { alarmHour = argInt("hour", -1); alarmMinute = argInt("minute", 0); alarmRamp = argInt("minutes", 30); lastAlarmDay = -1; ok("alarm"); });
  server.on("/timer", [] { int m = argInt("minutes", 0); offAt = m > 0 ? millis() + m * 60000UL : 0; ok("timer"); });
  server.begin();
}

// ---------------- SETUP AND LOOP ----------------
void setup() {
  Serial.begin(115200);
  strip.begin(); strip.show();
  panServo.setPeriodHertz(50); tiltServo.setPeriodHertz(50);
  panServo.attach(PAN_PIN, 500, 2400); tiltServo.attach(TILT_PIN, 500, 2400);
  writeServos(); lastHead = millis();
  if (USE_PIR) pinMode(PIR_PIN, INPUT);

  // touch baseline: don't touch the pad during the first second
  long sum = 0; for (int i = 0; i < 20; i++) { sum += touchRead(TOUCH_PIN); delay(50); }
  touchBaseline = sum / 20;

  // hello: a short warm glow and a nod
  setLightKelvin(2700, 25, 800);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Joining Wi-Fi");
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(250); Serial.print("."); updateLight(); }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nLamp ready at http://"); Serial.println(WiFi.localIP());
    configTzTime(TIMEZONE, "pool.ntp.org");
  } else {
    Serial.println("\nNo Wi-Fi. Touch and gestures still work; check WIFI_SSID/WIFI_PASSWORD.");
  }
  setupServer();
  setPose(0, 0, 40); startGesture("nod");
}

unsigned long lastSense = 0;
void loop() {
  server.handleClient();
  updateGesture();
  updateHead();
  updateSunrise();
  updateLight();
  if (millis() - lastSense > 50) { lastSense = millis(); updateSenses(); checkAlarmAndTimer(); }
  delay(10);
}
