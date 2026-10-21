#include "esp_camera.h"

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

#define LED_RED_PIN   12
#define LED_GREEN_PIN 13
#define LED_BLUE_PIN  14
#define BUZZER_PIN    15

const int PROXIMITY_THRESHOLD = 60; 

unsigned long badPostureStartTime = 0;
bool isBadPosture = false;
const unsigned long TIER_3_DELAY = 60000; 

void setup() {
  Serial.begin(115200);

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  setTier(1); 

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM; config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM; config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 10000000; 
  config.pixel_format = PIXFORMAT_GRAYSCALE; 
  config.frame_size = FRAMESIZE_QVGA; 
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera Ready! Distance measuring started...");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;

  long centerSum = 0;
  int pixelCount = 0;

  for (int y = 80; y < 160; y++) {
    for (int x = 100; x < 220; x++) {
      int index = y * 320 + x; 
      centerSum += fb->buf[index];
      pixelCount++;
    }
  }
  
  int avgCenterBrightness = centerSum / pixelCount;
  
  esp_camera_fb_return(fb); 

  Serial.print("Center Brightness (Distance factor): ");
  Serial.println(avgCenterBrightness);

  bool isTooClose = false;
  
  if (avgCenterBrightness < PROXIMITY_THRESHOLD) { 
    isTooClose = true; 
  }

  unsigned long currentTime = millis();

  if (isTooClose) {
    if (!isBadPosture) {
      isBadPosture = true;
      badPostureStartTime = currentTime;
      setTier(2); 
      Serial.println("Distance: Too Close! (Tier 2)");
    } else {
      if (currentTime - badPostureStartTime >= TIER_3_DELAY) {
        setTier(3); 
        Serial.println("Distance: Critical! (Tier 3)");
      }
    }
  } else {
    if (isBadPosture) {
      isBadPosture = false;
      setTier(1); 
      Serial.println("Distance: OK (Tier 1)");
    }
  }

  delay(1000); 
}

void setTier(int tier) {
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  if (tier == 1) {
    digitalWrite(LED_GREEN_PIN, HIGH);
  } 
  else if (tier == 2) {
    digitalWrite(LED_BLUE_PIN, HIGH);
  } 
  else if (tier == 3) {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }
}