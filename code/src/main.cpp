#include <Arduino.h>
#include <BleGamepad.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad
#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>

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

// init arrays
bool previousButtonStates[NUM_BUTTONS + NUM_ENC * 2];
bool currentButtonStates[NUM_BUTTONS + NUM_ENC * 2];

// init timers
unsigned long lastReportTime = 0;
unsigned long lastEncoderChangeTime = 0;
bool encoderChangeReported = 0;

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

    // init button pins
    initButtonPins();

    // init gamepad
    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setAutoReport(false);
    bleGamepadConfig.setButtonCount(NUM_BUTTONS + NUM_ENC * 2);
    bleGamepad.begin(&bleGamepadConfig);
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

void loop()
{
    if (bleGamepad.isConnected())
    {
        if (millis() - lastReportTime > 20)
        {
            lastReportTime = millis();
            encoderChangeReported = 1;
            updateButtonStatus();
        }
        bleGamepad.sendReport();
    }
    updateEncoderStatus(); // out of the loop to keep quicker updates
}
