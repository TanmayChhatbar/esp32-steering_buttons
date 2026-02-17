const int pins[] = {
    6, 7, 3, 1, 2, 5, 39, 34, // BUT*
    38, 37, 47, 48,           // BUTD*
    14, 9, 10,                // ENC1 was originally 8, 9, 10 according to design, but i burnt the solder pad
    11, 12, 13                // ENC2
};
#define NUMPINS (8 + 4 + 3 + 3)
int lastButtonState[NUMPINS] = {0};

#include <Arduino.h>
void setup()
{
    Serial.begin(115200);
    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        pinMode(pins[i], INPUT_PULLUP);
    }
    pinMode(48, OUTPUT);
}

void loop()
{
    int buttonState[NUMPINS] = {0};
    bool toPrint = false;
    for (int i = 0; i < NUMPINS; i++)
    {
        buttonState[i] = digitalRead(pins[i]);
        if (buttonState[i] != lastButtonState[i])
        {
            toPrint = true;
            lastButtonState[i] = buttonState[i];
        }
    }
    if (toPrint)
    {
        for (int i = 0; i < 8; i++)
        {
            Serial.print(buttonState[i]);
        }
        Serial.print(" ");
        for (int i = 8; i < 12; i++)
        {
            Serial.print(buttonState[i]);
        }
        Serial.print(" ");
        for (int i = 12; i < 15; i++)
        {
            Serial.print(buttonState[i]);
        }
        Serial.print(" ");
        for (int i = 15; i < sizeof(pins) / sizeof(pins[0]); i++)
        {
            Serial.print(buttonState[i]);
        }
        Serial.println();
    }
}
