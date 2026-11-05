#include <Arduino.h>
#include <BleGamepad.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad
#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include "credentials.h"
BleGamepad bleGamepad;

// normal buttons
#define NUM_BUTTONS (1 + 8 + 4 + 2)
const int pins[] = {
    6, 6, 7, 3, 1, 2, 5, 39, 34, // BUT* (first is dummy because windows doesnt report this, it seems)
    38, 37, 47, 48,              // BUTD*
    10, 13                       // ENC1, ENC2
};

// encoders
#define NUM_ENC 2
const int encPins[] = {
    14, 9, // ENC1 was originally 8, 9, 10 according to design, but i burnt the solder pad
    11, 12 // ENC2
};
Encoder enc[NUM_ENC] = {
    Encoder(encPins[0], encPins[1]),
    Encoder(encPins[2], encPins[3])};
int previousEncoderPositions[NUM_ENC] = {
    0, 0};
int currentEncoderPositions[NUM_ENC] = {
    0, 0};

// leds
#define LED_PIN 4
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];

// init arrays
bool previousButtonStates[NUM_BUTTONS + NUM_ENC * 2];
bool currentButtonStates[NUM_BUTTONS + NUM_ENC * 2];

// init timers
unsigned long lastReportTime = 0;
unsigned long lastEncoderChangeTime = 0;
bool encoderChangeReported = 0;

// OTA
bool OTAMode = false;

void blinkLEDs()
{
    static unsigned long last_led_update_time = 0;
    bool is_off = (leds[0] == CRGB::Black);
    unsigned long current_time = millis();
    if (current_time - last_led_update_time > 500)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            leds[i] = is_off ? CRGB::Yellow : CRGB::Black;
        }
        FastLED.show();
        last_led_update_time = current_time;
    }
}

void initButtonPins()
{
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        pinMode(pins[i], INPUT_PULLUP);
        previousButtonStates[i] = HIGH;
        currentButtonStates[i] = HIGH;
    }

    // init enc pins
    for (int i = 0; i < NUM_ENC * 2; i++)
    {
        pinMode(encPins[i], INPUT_PULLUP);
        previousButtonStates[NUM_BUTTONS + i] = HIGH;
        currentButtonStates[NUM_BUTTONS + i] = HIGH;
    }
}

void setup()
{
    Serial.begin(115200);

    // init led
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setDither(DISABLE_DITHER);
    FastLED.setBrightness(32);

    // init button pins
    initButtonPins();

    if (!digitalRead(pins[7]) && !digitalRead(pins[8]))
    {
        blinkLEDs();
        WiFi.begin(ssid, password);
        unsigned long long start_attempt_time = millis();
        while (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            blinkLEDs();
            if (millis() - start_attempt_time > 5000)
            {
                Serial.println("Failed to connect to WiFi. Restarting...");
                ESP.restart();
            }
            delay(100);
        }
        Serial.println("OTA Mode");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        ArduinoOTA.begin(); // Starts OTA
        OTAMode = true;
    }
    else
    {
        // init gamepad
        BleGamepadConfiguration bleGamepadConfig;
        bleGamepadConfig.setAutoReport(false);
        bleGamepadConfig.setButtonCount(NUM_BUTTONS + NUM_ENC * 2);

        // for feedback
        bleGamepadConfig.setEnableOutputReport(true);

        bleGamepad.begin(&bleGamepadConfig);
    }
}

void updateButtonStatus()
{
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        currentButtonStates[i] = digitalRead(pins[i]);

        if (currentButtonStates[i] != previousButtonStates[i])
        {
            if (currentButtonStates[i] == LOW)
            {
                bleGamepad.press(i);
                Serial.println("Pressed button " + String(i));
            }
            else
            {
                bleGamepad.release(i);
                Serial.println("Released button " + String(i));
            }
        }

        previousButtonStates[i] = currentButtonStates[i];
    }
}

void updateEncoderStatus()
{
    for (int i = 0; i < NUM_ENC; i++)
    {
        currentEncoderPositions[i] = enc[i].read() / 4;
    }
    if (millis() - lastEncoderChangeTime > 20)
    {
        for (int i = 0; i < NUM_ENC; i++)
        {
            if (currentEncoderPositions[i] > previousEncoderPositions[i])
            {
                // if position increased
                bleGamepad.press(NUM_BUTTONS + i * 2);
                previousEncoderPositions[i] = currentEncoderPositions[i];
                encoderChangeReported = 0;
                Serial.println("Encoder " + String(i) + " increased to " + String(currentEncoderPositions[i]));
            }
            else if (currentEncoderPositions[i] < previousEncoderPositions[i])
            {
                // if position decreased
                bleGamepad.press(NUM_BUTTONS + i * 2 + 1);
                previousEncoderPositions[i] = currentEncoderPositions[i];
                encoderChangeReported = 0;
                Serial.println("Encoder " + String(i) + " decreased to " + String(currentEncoderPositions[i]));
            }
            else if (encoderChangeReported)
            {
                // if position hasnt changed and last position was registered
                if (bleGamepad.isPressed(NUM_BUTTONS + i * 2))
                {
                    bleGamepad.release(NUM_BUTTONS + i * 2);
                    Serial.println("Encoder " + String(i) + " released");
                }
                if (bleGamepad.isPressed(NUM_BUTTONS + i * 2 + 1))
                {
                    bleGamepad.release(NUM_BUTTONS + i * 2 + 1);
                    Serial.println("Encoder " + String(i) + " released");
                }
            }
            previousEncoderPositions[i] = currentEncoderPositions[i];
            lastEncoderChangeTime = millis();
        }
    }
}

// config for leds
#define LEDS_SYMMETRIC false
#define LEDS_SMOOTH_TRANSITION true
#define LEDS_ALL_SAME false

#if LEDS_ALL_SAME
CRGB blendColors(const int *color_hues, const float *trigger_ratios, int num_colors, float rpm_ratio)
{
    // if at the last color, return it
    if (rpm_ratio >= trigger_ratios[num_colors - 1])
    {
        return CHSV(color_hues[num_colors - 1], 255, 255);
    }

    // find the indices of the two colors to blend between
    unsigned int i_plus = 0;
    for (i_plus = 0; i_plus < num_colors; i_plus++)
    {
        if (rpm_ratio < trigger_ratios[i_plus])
        {
            break;
        }
    }
    if (i_plus == 0)
    {
        CRGB color = CHSV(color_hues[0], 255, 255);
        float ratio = rpm_ratio / trigger_ratios[0];
        color.r = color.r * ratio;
        color.g = color.g * ratio;
        color.b = color.b * ratio;
        return color;
    }
    else
    {
        float ratio = (rpm_ratio - trigger_ratios[i_plus - 1]) / (trigger_ratios[i_plus] - trigger_ratios[i_plus - 1]);
        CRGB color = CHSV((color_hues[i_plus - 1] + int((color_hues[i_plus] - color_hues[i_plus - 1]) * ratio)), 255, 255);
        return color;
    }
}
#endif

void updateLEDs_RPM(unsigned int rpm, unsigned int max_rpm)
{
    // common config
    const float all_blue_ratio = 0.9f;
    const float flicker_ratio = 0.91f;
    const float flicker_ratio2 = 0.925f;

    // var init
    static unsigned long long last_flicker_time = 0;
    unsigned long long current_time = millis();
    float rpm_ratio = max((float)(rpm - 900) / (float)(max_rpm - 900), 0.0f);
    CRGB same_color = CRGB::Black;

#if LEDS_ALL_SAME
    const int num_leds = NUM_LEDS;
    const int color_hues[3] = {HUE_GREEN, HUE_YELLOW, HUE_RED};
    const float trigger_ratios[3] = {0.5, 0.675, 0.833};
    same_color = blendColors(color_hues, trigger_ratios, sizeof(color_hues) / sizeof(color_hues[0]), rpm_ratio);
#else
#if LEDS_SYMMETRIC
    const int num_leds = NUM_LEDS / (LEDS_SYMMETRIC ? 2 : 1);
    const CRGB colors[num_leds] = {CRGB::Green, CRGB::Green, CRGB::Yellow, CRGB::Red, CRGB::Red};
    const float trigger_ratios[num_leds] = {0.5, 0.6, 0.675, 0.75, 0.833};
#else
    const int num_leds = NUM_LEDS;
    const CRGB colors[num_leds] = {CRGB::Green, CRGB::Green, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Red, CRGB::Red};
    const float trigger_ratios[num_leds] = {0.5, 0.55, 0.6, 0.633, 0.667, 0.7, 0.733, 0.767, 0.8, 0.833};
#endif
#endif

    for (int i = 0; i < num_leds; i++)
    {
        if (rpm_ratio > all_blue_ratio)
        {
            leds[i] = CRGB::Blue;
        }
        else
        {
#if !LEDS_ALL_SAME
            if (LEDS_SMOOTH_TRANSITION)
            {
                if (rpm_ratio > trigger_ratios[i])
                {
                    leds[i] = (rpm_ratio > trigger_ratios[i]) ? colors[i] : CRGB::Black;
                }
                else if (i > 0 && rpm_ratio > trigger_ratios[i - 1])
                {
                    float ratio = (rpm_ratio - trigger_ratios[i - 1]) / (trigger_ratios[i] - trigger_ratios[i - 1]);
                    leds[i] = colors[i];
                    leds[i].r = leds[i].r * ratio;
                    leds[i].g = leds[i].g * ratio;
                    leds[i].b = leds[i].b * ratio;
                }
                else if (i == 0)
                {
                    float ratio = (rpm_ratio / trigger_ratios[i]);
                    leds[i] = colors[i];
                    leds[i].r = leds[i].r * ratio * ratio;
                    leds[i].g = leds[i].g * ratio * ratio;
                    leds[i].b = leds[i].b * ratio * ratio;
                }
                else
                {
                    leds[i] = CRGB::Black;
                }
            }
            else
            {
                leds[i] = (rpm_ratio > trigger_ratios[i]) ? colors[i] : CRGB::Black;
            }
#else
            {
                leds[i] = same_color;
            }
#endif
        }
    }

    // flicker at redline
    static bool flicker_on = true;
    if (rpm_ratio > flicker_ratio)
    {
        if (current_time - last_flicker_time >= 200 || (current_time - last_flicker_time >= 100 && rpm_ratio > flicker_ratio2))
        {
            flicker_on = !flicker_on;
            last_flicker_time = current_time;
        }

        if (!flicker_on)
        {
            for (int i = 0; i < num_leds; i++)
            {
                leds[i] = CRGB::Black;
            }
        }
    }
    else
    {
        flicker_on = true;
        last_flicker_time = current_time;
    }

    // mirror
#if !LEDS_ALL_SAME
    if (LEDS_SYMMETRIC)
    {
        for (int i = 0; i < num_leds; i++)
        {
            leds[NUM_LEDS - 1 - i] = leds[i];
        }
    }
#endif
    FastLED.show();
}

void loop()
{
    static unsigned long long ota_request_time = 0;
    static unsigned long long ouput_received_time = 0;
    unsigned long long current_time = millis();
    if (OTAMode)
    {
        ArduinoOTA.handle();
        blinkLEDs();
    }
    else
    {
        if (bleGamepad.isConnected())
        {
            if (current_time - lastReportTime > 20)
            {
                lastReportTime = current_time;
                encoderChangeReported = 1;
                updateButtonStatus();
            }
            bleGamepad.sendReport();

            // feedback
            if (bleGamepad.isOutputReceived())
            {
                uint8_t *buffer = bleGamepad.getOutputBuffer();

                int rpm = (buffer[0] << 8) | buffer[1];
                int max_rpm = (buffer[2] << 8) | buffer[3];

                updateLEDs_RPM(rpm, max_rpm);
                ouput_received_time = current_time;
            }
        }
        updateEncoderStatus(); // out of the loop to keep quicker updates

        // if no output received for 5 seconds, turn off leds
        if (current_time - ouput_received_time > 5000)
        {
            for (int i = 0; i < NUM_LEDS; i++)
            {
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }

        // go to OTA mode
        if (currentButtonStates[7] || currentButtonStates[8])
        {
            ota_request_time = current_time; // reset timer if either of the buttons is unpressed
        }
        if (current_time - ota_request_time > 3000)
        {
            Serial.println("OTA Mode requested. Restarting...");
            ESP.restart();
        }
    }
}
