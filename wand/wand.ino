/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* Includes ---------------------------------------------------------------- */
#include <DianaDing-project-1_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define LED_PIN 5           // D3 = GPIO5
#define BUTTON_PIN 21       // D6 = GPIO21 on XIAO ESP32C3

Adafruit_MPU6050 mpu;

#define SAMPLE_RATE_MS 10
#define CAPTURE_DURATION_MS 2000
#define FEATURE_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

float features[FEATURE_SIZE];
bool capturing = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;

bool lastButtonState = HIGH;

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void setup() {
    delay(500);  // 电池供电时稳定一下供电电压

    Serial.begin(115200);
    while (!Serial && millis() < 3000);  // 最多等 3 秒，防止无串口阻塞

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, LOW);

    // 启动后 LED 快闪 2 次，确认程序在运行
    for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }

    Serial.println("Initializing MPU6050...");
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) { delay(10); }
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 ready. Press the button to start capture.");
}

void loop() {
    int currentButtonState = digitalRead(BUTTON_PIN);

    if (lastButtonState == HIGH && currentButtonState == LOW && !capturing) {
        Serial.println("Button pressed, starting capture...");
        sample_count = 0;
        capturing = true;
        capture_start_time = millis();
        last_sample_time = millis();
    }
    lastButtonState = currentButtonState;

    if (capturing) {
        capture_accelerometer_data();
    }

    delay(10);  // 防抖
}

void capture_accelerometer_data() {
    if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
        last_sample_time = millis();

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        if (sample_count < FEATURE_SIZE / 3) {
            int idx = sample_count * 3;
            features[idx] = a.acceleration.x;
            features[idx + 1] = a.acceleration.y;
            features[idx + 2] = a.acceleration.z;
            sample_count++;
        }

        if (millis() - capture_start_time >= CAPTURE_DURATION_MS) {
            capturing = false;
            Serial.println("Capture complete.");
            run_inference();
        }
    }
}

void run_inference() {
    if (sample_count * 3 < FEATURE_SIZE) {
        Serial.println("ERROR: Not enough data for inference");
        return;
    }

    ei_impulse_result_t result = { 0 };
    signal_t features_signal;
    features_signal.total_length = FEATURE_SIZE;
    features_signal.get_data = &raw_feature_get_data;

    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier (");
        Serial.print(res);
        Serial.println(")");
        return;
    }

    print_inference_result(result);
}

void print_inference_result(ei_impulse_result_t result) {
    float max_value = 0;
    int max_index = -1;

    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_value) {
            max_value = result.classification[i].value;
            max_index = i;
        }
    }

    if (max_index != -1) {
        String label = ei_classifier_inferencing_categories[max_index];
        Serial.print("Prediction: ");
        Serial.print(label);
        Serial.print(" (");
        Serial.print(max_value * 100);
        Serial.println("%)");

        if (label == "O") {
            blink_led(3, 100);  // 快闪 3 次
        } else if (label == "V") {
            blink_led(2, 300);  // 慢闪 2 次
        } else if (label == "Z") {
            digitalWrite(LED_PIN, HIGH);
            delay(1000);
            digitalWrite(LED_PIN, LOW);
        } else {
            digitalWrite(LED_PIN, LOW); // 其他标签关闭 LED
        }
    }
}

void blink_led(int times, int delay_ms) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delay_ms);
        digitalWrite(LED_PIN, LOW);
        delay(delay_ms);
    }
}
