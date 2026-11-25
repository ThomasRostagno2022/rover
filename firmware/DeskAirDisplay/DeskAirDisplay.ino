/*
  Desk Air Display
  ----------------
  Elecrow CrowPanel Advance 4.3" (ESP32-S3, 800x480) + Adafruit SCD-41 CO2 sensor
  + Adafruit I2C rotary encoder (seesaw) + speaker.

  What it does
  - Shows room CO2 (green/amber/red), temperature and humidity.
  - Shows NYC weather for Hudson Square (Open-Meteo, no key needed).
  - A knob-driven timer with a chime.
  - Once an hour (7 AM - 9 PM ET) asks Claude for one sharp, relevant note
    plus a small drawing, and shows it in the big panel.
  - Do-not-disturb 9 PM - 7 AM ET: screen and knob light off, no Claude calls.
    Any touch or knob turn wakes the screen for 30 seconds.

  First start (or hold the knob down while powering on): the display creates a
  Wi-Fi network called "DeskAir-Setup". Join it from your phone and enter your
  Wi-Fi name, password and Claude API key. Nothing secret lives in this code.

  Arduino IDE settings (Tools menu):
    Board: ESP32S3 Dev Module
    Flash Size: 16MB (128Mb)
    PSRAM: OPI PSRAM
    Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
    USB CDC On Boot: Disabled   (this board's USB goes through a USB-to-serial chip)
  Libraries (Tools > Manage Libraries):
    LovyanGFX (lovyan03), Adafruit seesaw Library (+ Adafruit BusIO), ArduinoJson (Benoit Blanchon, v7)
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_seesaw.h>
#include <seesaw_neopixel.h>
#include <ESP_I2S.h>
#include <time.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_random.h"
#include "esp_heap_caps.h"
#include "LGFX_CrowPanel43.h"

// ============================================================
// Settings you might want to change
// ============================================================
static const char *DEFAULT_MODEL = "claude-sonnet-5-5";
static const char *CLAUDE_EFFORT = "medium";       // low | medium | high
static const bool USE_REFUSAL_FALLBACK = true;     // server-side fallback if Claude declines

static const float LAT = 40.7262f;                 // Hudson Square, Manhattan
static const float LON = -74.0080f;
static const char *PLACE = "Hudson Square, Manhattan, New York City";
static const char *TZ_INFO = "EST5EDT,M3.2.0,M11.1.0";   // America/New_York

static const int DND_START_HOUR = 21;              // 9 PM
static const int DND_END_HOUR = 7;                 // 7 AM
static const uint32_t DND_WAKE_MS = 30000;         // screen stays on this long after you touch it at night

static const uint8_t BACKLIGHT_DAY = 20;           // 0 = brightest, 245 = off
static const uint8_t BACKLIGHT_NIGHT_WAKE = 150;   // dimmer when woken at night
static const int KNOB_DIR = -1;                    // flip to 1 if the knob turns the wrong way

static const uint32_t WEATHER_EVERY_MS = 20UL * 60UL * 1000UL;
static const uint32_t ON_DEMAND_MIN_GAP_MS = 2UL * 60UL * 1000UL;

// ============================================================
// Board pins and addresses (from Elecrow's examples)
// ============================================================
static const int I2C_SDA = 15, I2C_SCL = 16;
static const uint8_t BOARD_CTRL = 0x30;            // backlight / amplifier helper chip
static const uint8_t ADDR_TOUCH = 0x5D;
static const uint8_t ADDR_SCD4X = 0x62;
static const uint8_t ADDR_KNOB = 0x36;
static const int SPK_BCLK = 5, SPK_LRCLK = 6, SPK_DATA = 4;
static const int SS_SWITCH = 24, SS_NEOPIX = 6;

// ============================================================
// Globals
// ============================================================
LGFX gfx;
LGFX_Sprite frame(&gfx);
Adafruit_seesaw knob;
seesaw_NeoPixel knobPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);
I2SClass i2s;
Preferences prefs;
SemaphoreHandle_t dataLock;

String cfgSsid, cfgPass, cfgKey, cfgModel;
bool haveKnob = false, haveScd = false, haveAudio = false;

// --- sensor ---
struct Air {
  bool valid = false;
  uint16_t co2 = 0;
  float tempF = 0, rh = 0;
  uint16_t maxToday = 0;
} air;
static const int HIST_N = 36;                      // one sample every 5 min = 3 h
uint16_t co2Hist[HIST_N];
int histCount = 0, histHead = 0;
uint32_t lastHistMs = 0;

// --- weather ---
struct Weather {
  bool valid = false;
  float tNow = 0, feels = 0, wind = 0, hi = 0, lo = 0;
  int code = 0;
  String sunset;          // "4:31 PM"
  String rainNote;        // "Rain likely 4 PM (70%)" or ""
  String next12;          // compact text for Claude
  uint32_t fetchedMs = 0;
} weather;

// --- Claude note ---
struct Shape {
  uint8_t kind;           // 0 rect, 1 roundrect, 2 circle, 3 line, 4 triangle
  int16_t x1, y1, x2, y2, x3, y3, r;
  uint16_t color;
  bool filled;
};
static const int MAX_SHAPES = 24;
struct Note {
  bool valid = false;
  String title, body, tone, timeLabel;
  Shape shapes[MAX_SHAPES];
  int nShapes = 0;
} note;
String recentNotes[3];
String netStatus = "Starting...";

// --- timer ---
enum Screen { HOME, SET, ALARM, SETUP_PORTAL };
Screen screen = HOME;
int setMin = 10;
bool tRunning = false, tPaused = false;
int32_t tRemainMs = 0, tTotalMs = 0;
uint32_t tLastMs = 0, alarmStartMs = 0, lastChimeMs = 0, setIdleMs = 0;
int timersToday = 0, focusMinToday = 0, lastYday = -1;

// --- display / interaction ---
volatile bool dirty = true;
uint32_t lastDrawMs = 0, lastInteractMs = 0;
bool screenOn = true;
uint8_t currentBacklight = 255;
String toastText;
uint32_t toastUntil = 0;
volatile bool claudeRequested = false;
uint32_t lastOnDemandMs = 0;

// shared snapshot for the network task
struct Snapshot {
  bool airValid; uint16_t co2, maxToday, co2HourAgo; float tempF, rh;
  String co2Trend;
  int timersToday, focusMin; bool tRunning; int tLeftMin, tTotalMin;
} snap;

// ============================================================
// Small helpers
// ============================================================
static inline uint16_t C(uint32_t rgb) { return lgfx::color565((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF); }

const uint16_t COL_BG = C(0x0E1117), COL_CARD = C(0x151B25), COL_LINE = C(0x222A35);
const uint16_t COL_INK = C(0xE8EDF3), COL_MUTED = C(0x8C97A6);
const uint16_t COL_GOOD = C(0x3FBF7F), COL_WARN = C(0xF0A63A), COL_BAD = C(0xEF5A4C), COL_INFO = C(0x5AA7E8);

bool timeValid(struct tm &t) { return getLocalTime(&t, 0) && t.tm_year > 120; }

bool isDnd() {
  struct tm t;
  if (!timeValid(t)) return false;
  return t.tm_hour >= DND_START_HOUR || t.tm_hour < DND_END_HOUR;
}

String hour12(int h, int m, bool withMin) {
  int hh = h % 12; if (hh == 0) hh = 12;
  char b[16];
  if (withMin) snprintf(b, sizeof b, "%d:%02d %s", hh, m, h < 12 ? "AM" : "PM");
  else snprintf(b, sizeof b, "%d %s", hh, h < 12 ? "AM" : "PM");
  return String(b);
}

// Keep only characters the built-in fonts can draw.
String asciiOnly(const String &in) {
  String out; out.reserve(in.length());
  for (size_t i = 0; i < in.length(); i++) {
    uint8_t c = in[i];
    if (c >= 32 && c < 127) { out += (char)c; continue; }
    if (c == 0xE2 && i + 2 < in.length()) {          // common UTF-8 punctuation
      uint8_t c2 = in[i + 1], c3 = in[i + 2];
      if (c2 == 0x80 && (c3 == 0x98 || c3 == 0x99)) out += '\'';
      else if (c2 == 0x80 && (c3 == 0x9C || c3 == 0x9D)) out += '"';
      else if (c2 == 0x80 && (c3 == 0x93 || c3 == 0x94)) out += '-';
      else if (c2 == 0x80 && c3 == 0xA6) out += "...";
      i += 2; continue;
    }
    if (c == 0xC2 && i + 1 < in.length() && (uint8_t)in[i + 1] == 0xB0) { out += " deg"; i++; continue; }
    // skip other multi-byte characters
    if ((c & 0xE0) == 0xC0) i += 1; else if ((c & 0xF0) == 0xE0) i += 2; else if ((c & 0xF8) == 0xF0) i += 3;
  }
  return out;
}

void setStatus(const String &s) {
  xSemaphoreTake(dataLock, portMAX_DELAY);
  netStatus = s;
  xSemaphoreGive(dataLock);
  dirty = true;
}

void toast(const String &s) { toastText = s; toastUntil = millis() + 1600; dirty = true; }

// ============================================================
// Board helper chip (backlight, amplifier)
// ============================================================
bool i2cPresent(uint8_t a) { Wire.beginTransmission(a); return Wire.endTransmission() == 0; }

bool boardCmd(uint8_t c) { Wire.beginTransmission(BOARD_CTRL); Wire.write(c); return Wire.endTransmission() == 0; }

void setBacklight(uint8_t level) {
  if (level == currentBacklight) return;
  currentBacklight = level;
  boardCmd(level);
}

void boardInit() {
  // Same start-up handshake as Elecrow's example, with a time limit.
  uint32_t start = millis();
  while (!(i2cPresent(BOARD_CTRL) && i2cPresent(ADDR_TOUCH)) && millis() - start < 4000) {
    boardCmd(250);                 // wake the touch controller
    pinMode(1, OUTPUT); digitalWrite(1, LOW); delay(120); pinMode(1, INPUT);
    delay(100);
  }
  Serial.printf("Board controller %s, touch %s\n", i2cPresent(BOARD_CTRL) ? "ok" : "MISSING", i2cPresent(ADDR_TOUCH) ? "ok" : "MISSING");
  setBacklight(BACKLIGHT_DAY);
}

// ============================================================
// SCD-41 (direct I2C, no library needed)
// ============================================================
uint8_t scdCrc(const uint8_t *d, int n) {
  uint8_t crc = 0xFF;
  for (int i = 0; i < n; i++) {
    crc ^= d[i];
    for (int b = 0; b < 8; b++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
  }
  return crc;
}
bool scdCmd(uint16_t cmd) {
  Wire.beginTransmission(ADDR_SCD4X); Wire.write(cmd >> 8); Wire.write(cmd & 0xFF);
  return Wire.endTransmission() == 0;
}
bool scdRead(uint16_t cmd, uint16_t *words, int n) {
  if (!scdCmd(cmd)) return false;
  delay(2);
  if (Wire.requestFrom((int)ADDR_SCD4X, n * 3) != n * 3) return false;
  for (int i = 0; i < n; i++) {
    uint8_t b[3] = { (uint8_t)Wire.read(), (uint8_t)Wire.read(), (uint8_t)Wire.read() };
    if (scdCrc(b, 2) != b[2]) return false;
    words[i] = ((uint16_t)b[0] << 8) | b[1];
  }
  return true;
}
void scdInit() {
  haveScd = i2cPresent(ADDR_SCD4X);
  if (!haveScd) { Serial.println("SCD-41 not found at 0x62"); return; }
  scdCmd(0x3F86); delay(500);     // stop any running measurement
  scdCmd(0x21B1);                 // start periodic measurement (new value every 5 s)
  Serial.println("SCD-41 started");
}
void scdPoll() {
  if (!haveScd) return;
  uint16_t ready;
  if (!scdRead(0xE4B8, &ready, 1) || (ready & 0x07FF) == 0) return;
  uint16_t w[3];
  if (!scdRead(0xEC05, w, 3)) return;
  if (w[0] == 0) return;
  air.co2 = w[0];
  float c = -45.0f + 175.0f * w[1] / 65535.0f;
  air.tempF = c * 9.0f / 5.0f + 32.0f;
  air.rh = 100.0f * w[2] / 65535.0f;
  air.valid = true;
  if (air.co2 > air.maxToday) air.maxToday = air.co2;
  if (histCount == 0 || millis() - lastHistMs >= 5UL * 60UL * 1000UL) {
    co2Hist[histHead] = air.co2; histHead = (histHead + 1) % HIST_N;
    if (histCount < HIST_N) histCount++;
    lastHistMs = millis();
  }
  dirty = true;
}
uint16_t histAgo(int samplesBack) {   // 0 = newest
  if (samplesBack >= histCount) return 0;
  return co2Hist[(histHead - 1 - samplesBack + HIST_N * 2) % HIST_N];
}

// ============================================================
// Speaker chime
// ============================================================
int16_t *chimeBuf = nullptr;
size_t chimeBytes = 0;

void audioInit() {
  const int rate = 16000;
  const int n1 = rate * 15 / 100, n2 = rate * 22 / 100;     // 150 ms + 220 ms
  const int n = n1 + n2;
  chimeBuf = (int16_t *)heap_caps_malloc(n * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM);
  if (!chimeBuf) return;
  for (int i = 0; i < n; i++) {
    bool first = i < n1;
    int k = first ? i : i - n1, len = first ? n1 : n2;
    float f = first ? 988.0f : 1319.0f;
    float env = min(1.0f, k / 160.0f) * (1.0f - (float)k / len);
    int16_t s = (int16_t)(sinf(2.0f * PI * f * k / rate) * 9000.0f * env);
    chimeBuf[i * 2] = 0;          // left unused
    chimeBuf[i * 2 + 1] = s;      // speaker is on the right channel
  }
  chimeBytes = n * 2 * sizeof(int16_t);
  i2s.setPins(SPK_BCLK, SPK_LRCLK, SPK_DATA);
  haveAudio = i2s.begin(I2S_MODE_STD, rate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  boardCmd(249);                  // keep amplifier muted between chimes
  Serial.printf("Speaker %s\n", haveAudio ? "ready" : "init failed");
}
void playChime() {
  if (!haveAudio || !chimeBuf) return;
  boardCmd(248);                  // unmute
  i2s.write((uint8_t *)chimeBuf, chimeBytes);
  delay(20);
  boardCmd(249);                  // mute
}

// ============================================================
// HTTPS (uses the ESP32's built-in root certificate bundle)
// ============================================================
int httpsRequest(const char *url, bool post, const String &body, const char *apiKey, bool beta, String &out) {
  esp_http_client_config_t cfg = {};
  cfg.url = url;
  cfg.method = post ? HTTP_METHOD_POST : HTTP_METHOD_GET;
  cfg.timeout_ms = 90000;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.buffer_size = 4096;
  cfg.buffer_size_tx = 2048;
  esp_http_client_handle_t c = esp_http_client_init(&cfg);
  if (!c) return -1;
  if (post) {
    esp_http_client_set_header(c, "content-type", "application/json");
    esp_http_client_set_header(c, "anthropic-version", "2026-10-21");
    if (apiKey) esp_http_client_set_header(c, "x-api-key", apiKey);
    if (beta) esp_http_client_set_header(c, "anthropic-beta", "server-side-fallback-2026-07-01");
  }
  int status = -1;
  if (esp_http_client_open(c, post ? body.length() : 0) == ESP_OK) {
    if (post) {
      size_t sent = 0;
      while (sent < body.length()) {
        int w = esp_http_client_write(c, body.c_str() + sent, body.length() - sent);
        if (w <= 0) break;
        sent += w;
      }
    }
    esp_http_client_fetch_headers(c);
    status = esp_http_client_get_status_code(c);
    out = "";
    char buf[1024];
    int r;
    while ((r = esp_http_client_read(c, buf, sizeof buf)) > 0) out.concat(buf, r);
  }
  esp_http_client_close(c);
  esp_http_client_cleanup(c);
  return status;
}

// ============================================================
// Weather (Open-Meteo, free, no key)
// ============================================================
const char *wxText(int code) {
  if (code == 0) return "clear";
  if (code == 1) return "mostly clear";
  if (code == 2) return "partly cloudy";
  if (code == 3) return "overcast";
  if (code == 45 || code == 48) return "fog";
  if (code >= 51 && code <= 57) return "drizzle";
  if (code >= 61 && code <= 67) return "rain";
  if (code >= 71 && code <= 77) return "snow";
  if (code >= 80 && code <= 82) return "showers";
  if (code == 85 || code == 86) return "snow showers";
  if (code >= 95) return "thunderstorms";
  return "mixed";
}

bool fetchWeather() {
  char url[512];
  snprintf(url, sizeof url,
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m"
           "&hourly=temperature_2m,precipitation_probability,weather_code"
           "&daily=temperature_2m_max,temperature_2m_min,sunset"
           "&timezone=America%%2FNew_York&forecast_days=2&temperature_unit=fahrenheit&wind_speed_unit=mph",
           LAT, LON);
  String resp;
  int st = httpsRequest(url, false, "", nullptr, false, resp);
  if (st != 200) { Serial.printf("Weather HTTP %d\n", st); return false; }
  JsonDocument d;
  if (deserializeJson(d, resp)) return false;

  Weather w;
  w.tNow = d["current"]["temperature_2m"] | 0.0f;
  w.feels = d["current"]["apparent_temperature"] | 0.0f;
  w.code = d["current"]["weather_code"] | 0;
  w.wind = d["current"]["wind_speed_10m"] | 0.0f;
  w.hi = d["daily"]["temperature_2m_max"][0] | 0.0f;
  w.lo = d["daily"]["temperature_2m_min"][0] | 0.0f;
  String ss = d["daily"]["sunset"][0] | "";          // "2026-11-25T16:31"
  if (ss.length() >= 16) w.sunset = hour12(ss.substring(11, 13).toInt(), ss.substring(14, 16).toInt(), true);

  String nowStr = d["current"]["time"] | "";
  String nowHour = nowStr.substring(0, 13);
  JsonArray times = d["hourly"]["time"];
  int start = 0;
  for (size_t i = 0; i < times.size(); i++) {
    String t = times[i] | "";
    if (t.substring(0, 13) >= nowHour) { start = i; break; }
  }
  String next;
  for (int i = start; i < start + 12 && i < (int)times.size(); i++) {
    String t = times[i] | "";
    int h = t.substring(11, 13).toInt();
    float tp = d["hourly"]["temperature_2m"][i] | 0.0f;
    int pop = d["hourly"]["precipitation_probability"][i] | 0;
    int code = d["hourly"]["weather_code"][i] | 0;
    char line[64];
    snprintf(line, sizeof line, "%s %.0fF %d%% %s; ", hour12(h, 0, false).c_str(), tp, pop, wxText(code));
    next += line;
    if (w.rainNote.length() == 0 && pop >= 40 && i > start) {
      char rn[48];
      snprintf(rn, sizeof rn, "%s likely %s (%d%%)", (code >= 71 && code <= 86) ? "Snow" : "Rain", hour12(h, 0, false).c_str(), pop);
      w.rainNote = rn;
    }
  }
  w.next12 = next;
  w.valid = true;
  w.fetchedMs = millis();

  xSemaphoreTake(dataLock, portMAX_DELAY);
  weather = w;
  xSemaphoreGive(dataLock);
  dirty = true;
  Serial.println("Weather updated");
  return true;
}

// ============================================================
// Claude
// ============================================================
static const char *SYSTEM_PROMPT =
  "You write the hourly panel for a small desk display in Hudson Square, Manhattan. "
  "The person reading it is working at that desk. Be sharp and relevant: give one specific, useful observation "
  "or nudge grounded only in the data provided - room air (CO2 trend, temperature, humidity), the next hours of "
  "weather, time of day and day of week, and their timer use. Favor what changes their next hour: when to step out "
  "before rain, when to air the room, a break after long focus, sunset and daylight, the evening commute. "
  "Use real numbers from the data. Never invent news, events, places or facts you were not given. "
  "No greetings, no motivational filler, no generic trivia, no emojis. Do not repeat or closely paraphrase the recent panels. "
  "Title: at most 32 characters. Body: at most 140 characters. Plain ASCII only. "
  "tone: alert for things needing action now (CO2 >= 1200, rain within the hour), nudge for a suggestion, info for a "
  "useful fact about the next hours, good when things are fine and worth confirming. "
  "Also draw a small, clear pictogram for the point with 3 to 14 shapes inside the canvas size you are given "
  "(origin top-left). Shape fields: rect and roundrect use (x1,y1)-(x2,y2) corners and r as corner radius; "
  "circle uses center (x1,y1) and radius r; line goes (x1,y1)-(x2,y2) with r as thickness 1-6; triangle uses "
  "(x1,y1),(x2,y2),(x3,y3). Set unused fields to 0. Colors are #RRGGBB and must read well on the dark background.";

static const char *NOTE_SCHEMA = R"JSON({
 "type":"object","additionalProperties":false,
 "required":["title","body","tone","shapes"],
 "properties":{
  "title":{"type":"string"},
  "body":{"type":"string"},
  "tone":{"type":"string","enum":["info","nudge","alert","good"]},
  "shapes":{"type":"array","items":{
    "type":"object","additionalProperties":false,
    "required":["kind","x1","y1","x2","y2","x3","y3","r","color","filled"],
    "properties":{
      "kind":{"type":"string","enum":["rect","roundrect","circle","line","triangle"]},
      "x1":{"type":"integer"},"y1":{"type":"integer"},
      "x2":{"type":"integer"},"y2":{"type":"integer"},
      "x3":{"type":"integer"},"y3":{"type":"integer"},
      "r":{"type":"integer"},
      "color":{"type":"string"},
      "filled":{"type":"boolean"}}}}}
})JSON";

// Canvas the drawing goes into (also used when drawing it).
static const int ART_X = 34, ART_Y = 166, ART_W = 290, ART_H = 290;

String buildContext() {
  Snapshot s; Weather w; String recent[3];
  xSemaphoreTake(dataLock, portMAX_DELAY);
  s = snap; w = weather;
  for (int i = 0; i < 3; i++) recent[i] = recentNotes[i];
  xSemaphoreGive(dataLock);

  struct tm t; getLocalTime(&t, 0);
  char date[48];
  strftime(date, sizeof date, "%A %b %d", &t);
  String ctx;
  ctx.reserve(1600);
  ctx += "Local time: " + String(date) + ", " + hour12(t.tm_hour, t.tm_min, true) + " (New York).\n";
  ctx += "Location: " + String(PLACE) + ".\n";
  if (s.airValid) {
    char b[200];
    snprintf(b, sizeof b, "Room air at the desk: CO2 %u ppm (1 hour ago %u, today's max %u), %.0fF, %.0f%% humidity.\n",
             s.co2, s.co2HourAgo, s.maxToday, s.tempF, s.rh);
    ctx += b;
    if (s.co2Trend.length()) ctx += "CO2 every 15 min, oldest first: " + s.co2Trend + "\n";
  } else ctx += "Room air: sensor not ready.\n";
  if (w.valid) {
    char b[220];
    snprintf(b, sizeof b, "Weather now: %.0fF (feels %.0fF), %s, wind %.0f mph. Today high %.0fF, low %.0fF. Sunset %s.\n",
             w.tNow, w.feels, wxText(w.code), w.wind, w.hi, w.lo, w.sunset.c_str());
    ctx += b;
    ctx += "Next 12 hours (time temp rain-chance sky): " + w.next12 + "\n";
  } else ctx += "Weather: unavailable.\n";
  ctx += "Timers today: " + String(s.timersToday) + " finished, " + String(s.focusMin) + " minutes total.";
  if (s.tRunning) ctx += " A timer is running: " + String(s.tLeftMin) + " of " + String(s.tTotalMin) + " min left.";
  ctx += "\nRecent panels (do not repeat):\n";
  bool any = false;
  for (int i = 0; i < 3; i++) if (recent[i].length()) { ctx += "- " + recent[i] + "\n"; any = true; }
  if (!any) ctx += "- none yet\n";
  ctx += "Drawing canvas: " + String(ART_W) + " x " + String(ART_H) + " px, background #151B25.\n";
  ctx += "Write this hour's panel.";
  return ctx;
}

uint16_t parseColor(const char *s) {
  if (!s || s[0] != '#' || strlen(s) < 7) return COL_INK;
  return C(strtoul(s + 1, nullptr, 16));
}

bool askClaude() {
  if (cfgKey.length() == 0) { setStatus("No Claude API key - hold knob at power-on to set up"); return false; }

  static bool fallbackOk = USE_REFUSAL_FALLBACK;
  String ctx = buildContext();

  for (int attempt = 0; attempt < 2; attempt++) {
    JsonDocument req;
    req["model"] = cfgModel;
    req["max_tokens"] = 4000;
    req["system"] = SYSTEM_PROMPT;
    req["output_config"]["effort"] = CLAUDE_EFFORT;
    req["output_config"]["format"]["type"] = "json_schema";
    req["output_config"]["format"]["schema"] = serialized(NOTE_SCHEMA);
    if (fallbackOk) req["fallbacks"] = "default";
    JsonArray msgs = req["messages"].to<JsonArray>();
    JsonObject m = msgs.add<JsonObject>();
    m["role"] = "user";
    m["content"] = ctx;
    String body;
    serializeJson(req, body);

    String resp;
    setStatus("Asking Claude...");
    int st = httpsRequest("https://api.anthropic.com/v1/messages", true, body, cfgKey.c_str(), fallbackOk, resp);
    Serial.printf("Claude HTTP %d, %u bytes\n", st, resp.length());

    if (st == 400 && fallbackOk && resp.indexOf("fallback") >= 0) { fallbackOk = false; continue; }  // retry without it
    if (st != 200) {
      JsonDocument e; deserializeJson(e, resp);
      String msg = e["error"]["message"] | "";
      if (st == 401) setStatus("Claude: API key rejected (401)");
      else if (st == 429) setStatus("Claude: rate limited, retrying later");
      else if (st < 0) setStatus("Claude: no connection");
      else setStatus("Claude error " + String(st) + (msg.length() ? ": " + msg.substring(0, 60) : ""));
      Serial.println(resp.substring(0, 400));
      return false;
    }

    JsonDocument filter;
    filter["stop_reason"] = true;
    filter["content"][0]["type"] = true;
    filter["content"][0]["text"] = true;
    JsonDocument d;
    if (deserializeJson(d, resp, DeserializationOption::Filter(filter))) { setStatus("Claude: unreadable reply"); return false; }
    String stop = d["stop_reason"] | "";
    if (stop == "refusal") { setStatus("Claude skipped this hour"); return false; }
    String text;
    for (JsonObject b : d["content"].as<JsonArray>()) {
      if (String((const char *)(b["type"] | "")) == "text") text = b["text"] | "";
    }
    JsonDocument nd;
    if (text.length() == 0 || deserializeJson(nd, text)) { setStatus("Claude: reply was not in the expected format"); return false; }

    Note n;
    n.title = asciiOnly(nd["title"] | "");
    n.body = asciiOnly(nd["body"] | "");
    n.tone = nd["tone"] | "info";
    for (JsonObject s : nd["shapes"].as<JsonArray>()) {
      if (n.nShapes >= MAX_SHAPES) break;
      Shape &sh = n.shapes[n.nShapes];
      String k = s["kind"] | "";
      sh.kind = k == "rect" ? 0 : k == "roundrect" ? 1 : k == "circle" ? 2 : k == "line" ? 3 : k == "triangle" ? 4 : 255;
      if (sh.kind == 255) continue;
      sh.x1 = constrain((int)(s["x1"] | 0), 0, ART_W); sh.y1 = constrain((int)(s["y1"] | 0), 0, ART_H);
      sh.x2 = constrain((int)(s["x2"] | 0), 0, ART_W); sh.y2 = constrain((int)(s["y2"] | 0), 0, ART_H);
      sh.x3 = constrain((int)(s["x3"] | 0), 0, ART_W); sh.y3 = constrain((int)(s["y3"] | 0), 0, ART_H);
      sh.r = constrain((int)(s["r"] | 0), 0, ART_W / 2);
      sh.color = parseColor(s["color"] | "#E8EDF3");
      sh.filled = s["filled"] | true;
      n.nShapes++;
    }
    struct tm t; getLocalTime(&t, 0);
    n.timeLabel = hour12(t.tm_hour, t.tm_min, true);
    n.valid = n.title.length() > 0;

    xSemaphoreTake(dataLock, portMAX_DELAY);
    note = n;
    recentNotes[2] = recentNotes[1]; recentNotes[1] = recentNotes[0];
    recentNotes[0] = n.title + " - " + n.body;
    xSemaphoreGive(dataLock);
    setStatus("");
    dirty = true;
    Serial.printf("Note: %s | %s\n", n.title.c_str(), n.body.c_str());
    return true;
  }
  return false;
}

// ============================================================
// Network task (runs alongside the screen)
// ============================================================
void netTask(void *) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfgSsid.c_str(), cfgPass.c_str());
  bool timeStarted = false;
  uint32_t nextWeather = 0, claudeRetryAt = 0;
  int lastClaudeHourKey = -1;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      setStatus("Connecting to Wi-Fi...");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (!timeStarted) {
      configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
      timeStarted = true;
      setStatus("");
    }
    uint32_t now = millis();
    if (now >= nextWeather) {
      nextWeather = now + (fetchWeather() ? WEATHER_EVERY_MS : 5UL * 60UL * 1000UL);
    }

    struct tm t;
    if (timeValid(t)) {
      int hourKey = t.tm_yday * 24 + t.tm_hour;
      bool dnd = isDnd();
      bool scheduled = !dnd && hourKey != lastClaudeHourKey && now >= claudeRetryAt;
      bool onDemand = claudeRequested;
      if (scheduled || onDemand) {
        claudeRequested = false;
        if (askClaude()) { lastClaudeHourKey = hourKey; claudeRetryAt = 0; }
        else if (scheduled) claudeRetryAt = millis() + 10UL * 60UL * 1000UL;
        dirty = true;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ============================================================
// Drawing
// ============================================================
void drawDeg(int x, int y, int r) { frame.drawCircle(x, y, r, COL_INK); frame.drawCircle(x, y, r - 1, COL_INK); }

uint16_t co2Color(uint16_t c) { return c >= 1200 ? COL_BAD : c >= 800 ? COL_WARN : COL_GOOD; }
const char *co2Label(uint16_t c) { return c >= 1200 ? "Open a window" : c >= 800 ? "Getting stuffy" : "Fresh"; }

String fmtMs(int32_t ms) {
  if (ms < 0) ms = 0;
  int s = (ms + 999) / 1000;
  char b[12]; snprintf(b, sizeof b, "%02d:%02d", s / 60, s % 60);
  return String(b);
}

// Word-wrap text into a box; returns the y after the last line.
int drawWrapped(const String &text, int x, int y, int w, int lineH, int maxLines) {
  String line, word;
  int lines = 0;
  auto flush = [&](bool last) {
    if (lines >= maxLines) return;
    if (lines == maxLines - 1 && !last) line += "...";
    frame.drawString(line, x, y + lines * lineH);
    lines++;
    line = "";
  };
  for (size_t i = 0; i <= text.length(); i++) {
    char c = i < text.length() ? text[i] : ' ';
    if (c == ' ') {
      String trial = line.length() ? line + " " + word : word;
      if (frame.textWidth(trial) > w && line.length()) { flush(false); line = word; }
      else line = trial;
      word = "";
      if (lines >= maxLines) break;
    } else word += c;
  }
  if (line.length() && lines < maxLines) flush(true);
  return y + lines * lineH;
}

void drawHome() {
  struct tm t; bool tv = timeValid(t);
  Weather w; Note n; String status;
  xSemaphoreTake(dataLock, portMAX_DELAY);
  w = weather; n = note; status = netStatus;
  xSemaphoreGive(dataLock);

  frame.fillScreen(COL_BG);

  // --- top bar ---
  frame.setTextDatum(textdatum_t::middle_left);
  uint16_t cc = air.valid ? co2Color(air.co2) : COL_MUTED;
  frame.fillCircle(34, 50, 11, cc);
  frame.setTextColor(COL_INK);
  frame.setFont(&fonts::FreeSansBold24pt7b); frame.setTextSize(1.25f);
  String co2s = air.valid ? String(air.co2) : String("----");
  frame.drawString(co2s, 56, 50);
  int x = 56 + frame.textWidth(co2s) + 12;
  frame.setTextSize(1);
  frame.setFont(&fonts::FreeSans9pt7b); frame.setTextColor(COL_MUTED);
  frame.drawString("ppm CO2", x, 38);
  frame.setFont(&fonts::FreeSansBold9pt7b); frame.setTextColor(COL_INK);
  frame.drawString(air.valid ? co2Label(air.co2) : (haveScd ? "Warming up" : "No sensor"), x, 62);

  // room temp / humidity
  frame.setFont(&fonts::FreeSansBold12pt7b);
  if (air.valid) {
    String tt = String(air.tempF, 0);
    frame.drawString(tt, 330, 36);
    int tw = frame.textWidth(tt);
    drawDeg(330 + tw + 5, 29, 3);
    frame.drawString("F", 330 + tw + 11, 36);
    frame.drawString(String(air.rh, 0) + "%", 330, 66);
  }
  frame.setFont(&fonts::FreeSans9pt7b); frame.setTextColor(COL_MUTED);
  frame.drawString("room", 400, 36);
  frame.drawString("humidity", 400, 66);

  // timer chip
  if (tRunning) {
    frame.fillRoundRect(486, 30, 150, 40, 20, C(0x1C2532));
    float frac = tTotalMs > 0 ? 1.0f - (float)tRemainMs / tTotalMs : 0;
    frame.fillRect(500, 64, (int)(122 * frac), 3, COL_WARN);
    frame.setFont(&fonts::FreeSansBold12pt7b); frame.setTextColor(tPaused ? COL_MUTED : COL_INK);
    frame.setTextDatum(textdatum_t::middle_center);
    frame.drawString(tPaused ? "II " + fmtMs(tRemainMs) : fmtMs(tRemainMs), 561, 48);
    frame.setTextDatum(textdatum_t::middle_left);
  }

  // clock
  frame.setTextDatum(textdatum_t::middle_right);
  frame.setTextColor(COL_INK);
  if (tv) {
    int hh = t.tm_hour % 12; if (hh == 0) hh = 12;
    char b[8]; snprintf(b, sizeof b, "%d:%02d", hh, t.tm_min);
    frame.setFont(&fonts::FreeSansBold9pt7b); frame.setTextColor(COL_MUTED);
    frame.drawString(t.tm_hour < 12 ? "AM" : "PM", 778, 58);
    frame.setFont(&fonts::FreeSansBold24pt7b); frame.setTextColor(COL_INK);
    frame.drawString(b, 746, 50);
  } else {
    frame.setFont(&fonts::FreeSans12pt7b); frame.setTextColor(COL_MUTED);
    frame.drawString("--:--", 778, 50);
  }
  frame.drawFastHLine(0, 98, 800, COL_LINE);

  // --- weather strip ---
  frame.setTextDatum(textdatum_t::middle_left);
  frame.setFont(&fonts::FreeSans12pt7b);
  int wx = 24;
  if (w.valid) {
    frame.setTextColor(COL_INK);
    String now = String(w.tNow, 0);
    frame.drawString(now, wx, 124); wx += frame.textWidth(now) + 4;
    drawDeg(wx + 3, 116, 3); wx += 12;
    String rest = String(wxText(w.code)) + "  feels " + String(w.feels, 0) + "  H " + String(w.hi, 0) + " L " + String(w.lo, 0);
    frame.drawString(rest, wx, 124); wx += frame.textWidth(rest) + 22;
    frame.setTextColor(w.rainNote.length() ? COL_INFO : COL_MUTED);
    String extra = w.rainNote.length() ? w.rainNote : ("Sunset " + w.sunset);
    frame.drawString(extra, wx, 124);
  } else {
    frame.setTextColor(COL_MUTED);
    frame.drawString("Weather loading...", wx, 124);
  }

  // --- Claude panel ---
  frame.fillRoundRect(20, 152, 760, 314, 14, COL_CARD);
  if (n.valid) {
    for (int i = 0; i < n.nShapes; i++) {
      const Shape &s = n.shapes[i];
      int ox = ART_X, oy = ART_Y;
      switch (s.kind) {
        case 0: { int x0 = min(s.x1, s.x2), y0 = min(s.y1, s.y2), ww = abs(s.x2 - s.x1), hh = abs(s.y2 - s.y1);
                  if (s.filled) frame.fillRect(ox + x0, oy + y0, ww, hh, s.color); else frame.drawRect(ox + x0, oy + y0, ww, hh, s.color); } break;
        case 1: { int x0 = min(s.x1, s.x2), y0 = min(s.y1, s.y2), ww = abs(s.x2 - s.x1), hh = abs(s.y2 - s.y1);
                  int rr = min((int)s.r, min(ww, hh) / 2);
                  if (s.filled) frame.fillRoundRect(ox + x0, oy + y0, ww, hh, rr, s.color); else frame.drawRoundRect(ox + x0, oy + y0, ww, hh, rr, s.color); } break;
        case 2: if (s.filled) frame.fillCircle(ox + s.x1, oy + s.y1, s.r, s.color); else frame.drawCircle(ox + s.x1, oy + s.y1, s.r, s.color); break;
        case 3: frame.drawWideLine(ox + s.x1, oy + s.y1, ox + s.x2, oy + s.y2, max(1, min((int)s.r, 6)) / 2.0f + 0.5f, s.color); break;
        case 4: if (s.filled) frame.fillTriangle(ox + s.x1, oy + s.y1, ox + s.x2, oy + s.y2, ox + s.x3, oy + s.y3, s.color);
                else frame.drawTriangle(ox + s.x1, oy + s.y1, ox + s.x2, oy + s.y2, ox + s.x3, oy + s.y3, s.color); break;
      }
    }
    uint16_t toneCol = n.tone == "alert" ? COL_BAD : n.tone == "nudge" ? COL_WARN : n.tone == "good" ? COL_GOOD : COL_INFO;
    frame.fillRoundRect(350, 190, 6, 40, 3, toneCol);
    frame.setTextDatum(textdatum_t::top_left);
    frame.setTextColor(COL_INK);
    frame.setFont(&fonts::FreeSansBold18pt7b);
    int y = drawWrapped(n.title, 368, 188, 390, 40, 2);
    frame.setFont(&fonts::FreeSans12pt7b); frame.setTextColor(C(0xC9D2DE));
    drawWrapped(n.body, 368, y + 14, 390, 30, 5);
    frame.setFont(&fonts::FreeSans9pt7b); frame.setTextColor(C(0x5F6B7A));
    frame.setTextDatum(textdatum_t::bottom_right);
    frame.drawString("Claude  " + n.timeLabel, 764, 456);
  } else {
    frame.setTextDatum(textdatum_t::middle_center);
    frame.setFont(&fonts::FreeSans12pt7b); frame.setTextColor(COL_MUTED);
    frame.drawString(isDnd() ? "Quiet hours - Claude is back at 7 AM" : "Claude's note will appear here", 400, 300);
  }
  if (status.length()) {
    frame.setTextDatum(textdatum_t::bottom_left);
    frame.setFont(&fonts::FreeSans9pt7b); frame.setTextColor(COL_WARN);
    frame.drawString(status, 36, 456);
  }
}

void drawSet() {
  frame.fillScreen(COL_BG);
  int cx = 400, cy = 205;
  frame.fillArc(cx, cy, 150, 136, 0, 360, COL_LINE);
  float frac = min(setMin, 60) / 60.0f;
  if (frac > 0) frame.fillArc(cx, cy, 150, 136, -90, -90 + 360.0f * frac, COL_WARN);
  frame.setTextDatum(textdatum_t::middle_center);
  frame.setTextColor(COL_INK);
  frame.setFont(&fonts::FreeSansBold24pt7b); frame.setTextSize(2.0f);
  char b[8]; snprintf(b, sizeof b, "%02d:00", setMin);
  frame.drawString(b, cx, cy - 6);
  frame.setTextSize(1);
  frame.setFont(&fonts::FreeSans12pt7b); frame.setTextColor(COL_MUTED);
  frame.drawString(setMin == 0 ? "turn to set" : String(setMin) + (setMin == 1 ? " minute" : " minutes"), cx, cy + 62);
  const int presets[5] = { 5, 10, 15, 25, 45 };
  for (int i = 0; i < 5; i++) {
    int px = 240 + i * 80;
    bool on = presets[i] == setMin;
    frame.drawRoundRect(px - 30, 384, 60, 34, 17, on ? COL_WARN : COL_LINE);
    frame.setTextColor(on ? COL_INK : COL_MUTED);
    frame.drawString(String(presets[i]), px, 401);
  }
  frame.setFont(&fonts::FreeSans9pt7b); frame.setTextColor(COL_MUTED);
  frame.drawString("Press to start  -  Hold to cancel", cx, 448);
}

void drawAlarm() {
  bool on = (millis() / 450) % 2;
  frame.fillScreen(on ? C(0xB4561F) : C(0x3A1D12));
  frame.setTextDatum(textdatum_t::middle_center);
  frame.setTextColor(COL_INK);
  frame.setFont(&fonts::FreeSansBold24pt7b); frame.setTextSize(2.0f);
  frame.drawString("Time's up", 400, 210);
  frame.setTextSize(1);
  frame.setFont(&fonts::FreeSans12pt7b); frame.setTextColor(C(0xFFE7D2));
  frame.drawString(String(tTotalMs / 60000) + " min timer  -  press the knob to stop", 400, 290);
}

void drawToast() {
  if (millis() > toastUntil) return;
  frame.setFont(&fonts::FreeSansBold12pt7b);
  int w = frame.textWidth(toastText) + 40;
  frame.fillRoundRect(400 - w / 2, 410, w, 40, 20, C(0xE8EDF3));
  frame.setTextColor(C(0x10141A));
  frame.setTextDatum(textdatum_t::middle_center);
  frame.drawString(toastText, 400, 430);
}

void render() {
  if (screen == HOME) drawHome();
  else if (screen == SET) drawSet();
  else if (screen == ALARM) drawAlarm();
  drawToast();
  frame.pushSprite(0, 0);
  lastDrawMs = millis();
  dirty = false;
}

// ============================================================
// Timer logic
// ============================================================
void startTimer() {
  if (setMin <= 0) { toast("Turn the knob to pick a time"); return; }
  tTotalMs = tRemainMs = setMin * 60000;
  tRunning = true; tPaused = false; tLastMs = millis();
  screen = HOME; toast("Timer started: " + String(setMin) + " min");
}
void cancelTimer() {
  bool had = tRunning || screen == SET;
  tRunning = tPaused = false; screen = HOME;
  if (had) toast("Timer cancelled");
}
void startAlarm() {
  tRunning = false;
  timersToday++; focusMinToday += tTotalMs / 60000;
  screen = ALARM; alarmStartMs = millis(); lastChimeMs = 0;
  lastInteractMs = millis();     // light the screen even at night
}
void stopAlarm() { screen = HOME; toast("Alarm stopped"); }

void updateTimer() {
  uint32_t now = millis();
  if (tRunning && !tPaused) {
    tRemainMs -= (int32_t)(now - tLastMs);
    if (tRemainMs <= 0) { tRemainMs = 0; startAlarm(); }
  }
  tLastMs = now;
  if (screen == ALARM) {
    if (now - lastChimeMs > 1200) { playChime(); lastChimeMs = millis(); }
    if (now - alarmStartMs > 60000) stopAlarm();
    dirty = true;
  }
  if (screen == SET && now - setIdleMs > 7000) { screen = HOME; toast("Not started - press the knob to start"); }
}

// ============================================================
// Knob + touch input
// ============================================================
bool btnDown = false, longFired = false, clickPending = false;
uint32_t btnDownMs = 0, clickMs = 0, lastStepMs = 0;

// Returns true if the input should only wake the screen (night, screen off).
bool wakeOnly() {
  bool wasOff = !screenOn;
  lastInteractMs = millis();
  if (wasOff) { dirty = true; return true; }
  return false;
}

void onTurn(int steps) {
  if (wakeOnly()) return;
  uint32_t now = millis();
  int d = steps * ((now - lastStepMs < 60) ? 5 : 1);
  lastStepMs = now;
  if (screen == ALARM) return;
  if (screen == HOME) {
    if (tRunning) {
      tRemainMs = max<int32_t>(1000, tRemainMs + d * 60000);
      if (tRemainMs > tTotalMs) tTotalMs = tRemainMs;
      toast((d > 0 ? "+" : "") + String(d) + " min");
      return;
    }
    screen = SET;
  } else {
    setMin = constrain(setMin + d, 0, 120);
  }
  setIdleMs = now; dirty = true;
}
void onPress() {
  if (wakeOnly()) return;
  if (screen == ALARM) { stopAlarm(); return; }
  if (screen == SET) { startTimer(); return; }
  if (tRunning) { tPaused = !tPaused; toast(tPaused ? "Paused" : "Resumed"); return; }
  screen = SET; setIdleMs = millis(); dirty = true;
}
void onLongPress() {
  if (wakeOnly()) return;
  if (screen == ALARM) { stopAlarm(); return; }
  cancelTimer();
}
void onDoublePress() {
  if (wakeOnly()) return;
  if (isDnd()) { toast("Quiet hours - no new notes until 7 AM"); return; }
  if (millis() - lastOnDemandMs < ON_DEMAND_MIN_GAP_MS && lastOnDemandMs) { toast("New note available in a minute or two"); return; }
  lastOnDemandMs = millis();
  claudeRequested = true;
  toast("Asking Claude for a new note...");
}

void pollKnob() {
  if (!haveKnob) return;
  int32_t delta = knob.getEncoderDelta();
  if (delta) onTurn(delta * KNOB_DIR);

  bool down = !knob.digitalRead(SS_SWITCH);
  uint32_t now = millis();
  if (down && !btnDown) { btnDown = true; btnDownMs = now; longFired = false; }
  if (down && btnDown && !longFired && now - btnDownMs > 800) { longFired = true; clickPending = false; onLongPress(); }
  if (!down && btnDown) {
    btnDown = false;
    if (!longFired && now - btnDownMs > 20) {
      bool canDouble = screen == HOME && !tRunning && screenOn;
      if (!canDouble) onPress();
      else if (clickPending && now - clickMs < 320) { clickPending = false; onDoublePress(); }
      else { clickPending = true; clickMs = now; }
    }
  }
  if (clickPending && now - clickMs >= 320) { clickPending = false; onPress(); }
}

void pollTouch() {
  static uint32_t last = 0;
  if (millis() - last < 60) return;
  last = millis();
  lgfx::touch_point_t tp;
  if (gfx.getTouch(&tp, 1)) { if (!screenOn) dirty = true; lastInteractMs = millis(); }
}

// ============================================================
// Night mode, knob light, daily reset, snapshot for Claude
// ============================================================
void updateScreenPower() {
  bool dnd = isDnd();
  bool awake = !dnd || screen == ALARM || millis() - lastInteractMs < DND_WAKE_MS;
  if (awake != screenOn) { screenOn = awake; dirty = true; }
  setBacklight(screenOn ? (dnd ? BACKLIGHT_NIGHT_WAKE : BACKLIGHT_DAY) : 245);

  static uint32_t lastPix = 0; static uint32_t lastColor = 1;
  if (haveKnob && millis() - lastPix > 2000) {
    lastPix = millis();
    uint32_t col = 0;
    if (screenOn && air.valid) col = air.co2 >= 1200 ? 0xFF1E10 : air.co2 >= 800 ? 0xFF8C00 : 0x00C850;
    if (col != lastColor) { knobPixel.setPixelColor(0, (col >> 16) & 0xFF, (col >> 8) & 0xFF, col & 0xFF); knobPixel.show(); lastColor = col; }
  }
}

void dailyReset() {
  struct tm t;
  if (!timeValid(t)) return;
  if (lastYday != -1 && t.tm_yday != lastYday) { timersToday = 0; focusMinToday = 0; air.maxToday = air.co2; }
  lastYday = t.tm_yday;
}

void updateSnapshot() {
  Snapshot s;
  s.airValid = air.valid; s.co2 = air.co2; s.maxToday = air.maxToday; s.tempF = air.tempF; s.rh = air.rh;
  s.co2HourAgo = histAgo(12) ? histAgo(12) : air.co2;
  String trend;
  for (int i = 24; i >= 0; i -= 3) { uint16_t v = histAgo(i); if (v) trend += String(v) + " "; }
  s.co2Trend = trend;
  s.timersToday = timersToday; s.focusMin = focusMinToday;
  s.tRunning = tRunning; s.tLeftMin = (tRemainMs + 59999) / 60000; s.tTotalMin = tTotalMs / 60000;
  xSemaphoreTake(dataLock, portMAX_DELAY);
  snap = s;
  xSemaphoreGive(dataLock);
}

// ============================================================
// Setup portal (phone-based Wi-Fi + API key entry)
// ============================================================
WebServer *web = nullptr;
DNSServer *dns = nullptr;
String apPass;

String htmlEscape(const String &s) {
  String o;
  for (char c : s) { if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;"; else if (c == '&') o += "&amp;"; else if (c == '"') o += "&quot;"; else o += c; }
  return o;
}

void handleRoot() {
  String page =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>Desk Air setup</title><style>body{font-family:system-ui;max-width:460px;margin:24px auto;padding:0 16px}"
    "label{display:block;margin-top:14px;font-weight:600}input{width:100%;padding:10px;font-size:16px;box-sizing:border-box}"
    "button{margin-top:20px;padding:12px 18px;font-size:16px}small{color:#666}</style>"
    "<h2>Desk Air setup</h2><form method=post action=/save>"
    "<label>Wi-Fi name (2.4 GHz)</label><input name=ssid value=\"" + htmlEscape(cfgSsid) + "\" required>"
    "<label>Wi-Fi password</label><input name=pass type=password placeholder='" + String(cfgPass.length() ? "leave blank to keep" : "") + "'>"
    "<label>Claude API key</label><input name=key type=password placeholder='" + String(cfgKey.length() ? "saved - leave blank to keep" : "sk-ant-...") + "'>"
    "<label>Claude model</label><input name=model value=\"" + htmlEscape(cfgModel) + "\">"
    "<small>e.g. claude-sonnet-5-5, claude-opus-5-5, claude-haiku-5-5</small>"
    "<br><button>Save and restart</button></form>";
  web->send(200, "text/html", page);
}
void handleSave() {
  prefs.begin("deskair", false);
  if (web->arg("ssid").length()) prefs.putString("ssid", web->arg("ssid"));
  if (web->arg("pass").length()) prefs.putString("pass", web->arg("pass"));
  if (web->arg("key").length()) prefs.putString("key", web->arg("key"));
  if (web->arg("model").length()) prefs.putString("model", web->arg("model"));
  prefs.end();
  web->send(200, "text/html", "<meta name=viewport content='width=device-width'><h3>Saved. The display is restarting.</h3>");
  delay(1500);
  ESP.restart();
}

void startPortal() {
  screen = SETUP_PORTAL;
  char pw[9];
  snprintf(pw, sizeof pw, "%08lu", (unsigned long)(esp_random() % 100000000UL));
  apPass = pw;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("DeskAir-Setup", apPass.c_str());
  dns = new DNSServer();
  dns->start(53, "*", WiFi.softAPIP());
  web = new WebServer(80);
  web->on("/", handleRoot);
  web->on("/save", HTTP_POST, handleSave);
  web->onNotFound([] { web->sendHeader("Location", "http://192.168.4.1/"); web->send(302, "text/plain", ""); });
  web->begin();

  frame.fillScreen(COL_BG);
  frame.setTextDatum(textdatum_t::top_left);
  frame.setTextColor(COL_INK);
  frame.setFont(&fonts::FreeSansBold18pt7b);
  frame.drawString("Set up Desk Air", 40, 40);
  frame.setFont(&fonts::FreeSans12pt7b);
  frame.drawString("1. On your phone, join this Wi-Fi network:", 40, 120);
  frame.setFont(&fonts::FreeSansBold18pt7b);
  frame.drawString("DeskAir-Setup", 70, 160);
  frame.setFont(&fonts::FreeSans12pt7b);
  frame.drawString("Password:", 70, 214);
  frame.setFont(&fonts::FreeSansBold18pt7b);
  frame.drawString(apPass.substring(0, 4) + " " + apPass.substring(4), 200, 206);
  frame.setFont(&fonts::FreeSans12pt7b);
  frame.drawString("2. A setup page should open. If not, go to 192.168.4.1", 40, 290);
  frame.drawString("3. Enter your Wi-Fi and Claude API key, then Save.", 40, 340);
  frame.setTextColor(COL_MUTED);
  frame.drawString("To come back here later: hold the knob down while plugging in.", 40, 410);
  frame.pushSprite(0, 0);
  Serial.println("Setup portal running");
}

// ============================================================
// Arduino entry points
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nDesk Air Display starting");
  dataLock = xSemaphoreCreateMutex();

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(50);
  boardInit();

  gfx.init();
  gfx.fillScreen(TFT_BLACK);
  frame.setColorDepth(16);
  frame.setPsram(true);
  if (!frame.createSprite(800, 480)) Serial.println("ERROR: could not allocate frame buffer (is PSRAM set to OPI?)");

  haveKnob = knob.begin(ADDR_KNOB);
  if (haveKnob) {
    knob.pinMode(SS_SWITCH, INPUT_PULLUP);
    knob.getEncoderDelta();      // clear
    knobPixel.begin(ADDR_KNOB);
    knobPixel.setBrightness(25);
    knobPixel.setPixelColor(0, 0); knobPixel.show();
  }
  Serial.printf("Knob %s\n", haveKnob ? "ok" : "NOT FOUND at 0x36");

  scdInit();
  audioInit();

  prefs.begin("deskair", true);
  cfgSsid = prefs.getString("ssid", "");
  cfgPass = prefs.getString("pass", "");
  cfgKey = prefs.getString("key", "");
  cfgModel = prefs.getString("model", DEFAULT_MODEL);
  prefs.end();

  bool holdForSetup = haveKnob && !knob.digitalRead(SS_SWITCH);
  if (cfgSsid.length() == 0 || holdForSetup) { startPortal(); return; }

  xTaskCreatePinnedToCore(netTask, "net", 16384, nullptr, 1, nullptr, 0);
  lastInteractMs = millis();
  render();
}

void loop() {
  if (screen == SETUP_PORTAL) {
    dns->processNextRequest();
    web->handleClient();
    delay(5);
    return;
  }

  pollKnob();
  pollTouch();

  static uint32_t lastSensor = 0, lastSnap = 0;
  uint32_t now = millis();
  if (now - lastSensor > 1000) { lastSensor = now; scdPoll(); dailyReset(); }
  if (now - lastSnap > 5000) { lastSnap = now; updateSnapshot(); }

  updateTimer();
  updateScreenPower();

  if (screenOn) {
    uint32_t every = (screen == ALARM || millis() < toastUntil + 100) ? 150 : (screen == SET ? 120 : 1000);
    if (dirty || now - lastDrawMs >= every) render();
  }
  delay(15);
}
