#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "env.h"
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <RBDdimmer.h>

// wifi
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WiFiClient client;

// thing speak
unsigned long channel_ID = SECRET_CH_ID;
const char *apiKey = SECRET_WRITE_APIKEY;

// millis
unsigned long cetakPerDetik = 0;

// lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

// dht
#define DHT1_PIN 33
#define DHT2_PIN 27
#define DHTTYPE DHT22
DHT dht1(DHT1_PIN, DHTTYPE);
DHT dht2(DHT2_PIN, DHTTYPE);
float in_temp1;
float in_temp2;
float in_hum1;
float in_hum2;

// dimmer
#define outDimmer 32
#define ZC 35
dimmerLamp dimmer(outDimmer, ZC);
float out_heater;

// l298n
#define IN1 15
#define IN2 14
#define ENA 4
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
float out_fan;

// fuzzy inference system
const int NUM_INPUTS = 4;
const int NUM_SETS = 3;
const int NUM_RULES = 81;

float membershipFunction(float x, float a, float b, float c)
{
    if (x <= a || x >= c)
        return 0.0;
    else if (x <= b)
        return (x - a) / (b - a);
    else
        return (c - x) / (c - b);
}

float fuzzySets[NUM_INPUTS][NUM_SETS][3] = {
    {{25, 27.5, 30}, {27.5, 30, 32.5}, {30, 32.5, 35}},  // Suhu1
    {{25, 27.5, 30}, {27.5, 30, 32.5}, {30, 32.5, 35}},  // Suhu2
    {{40, 52.5, 65}, {52.5, 65, 77.5}, {65, 77.5, 90}},  // Kelembapan1
    {{40, 52.5, 65}, {52.5, 65, 77.5}, {65, 77.5, 90}}}; // Kelembapan2

float ruleConsequents[NUM_RULES][2] = {
    {20, 150},  // Rule 0: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 1: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 2: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 3: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 4: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 5: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 6: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 7: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 8: Suhu1 is Sejuk1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 9: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 10: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 11: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 12: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 13: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 14: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 15: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 16: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 17: Suhu1 is Sejuk1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 18: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 19: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 20: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 21: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 22: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 23: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 24: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 25: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 26: Suhu1 is Sejuk1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 27: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 28: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 29: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 30: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 31: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 32: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 33: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 34: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 35: Suhu1 is Hangat1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 36: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 37: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 38: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 39: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 40: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 41: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 42: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 43: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 44: Suhu1 is Hangat1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 45: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 46: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 47: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 48: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 49: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 50: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 51: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 52: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 53: Suhu1 is Hangat1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 54: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 55: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 56: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 57: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 58: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 59: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 60: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 61: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 62: Suhu1 is Panas1, Suhu2 is Sejuk2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 63: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 64: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 65: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 66: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 67: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 68: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 69: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 70: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 71: Suhu1 is Panas1, Suhu2 is Hangat2, Kelembapan1 is Kering1, Kelembapan2 is Kering2

    {20, 150},  // Rule 72: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Basah2
    {50, 190},  // Rule 73: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Lembab2
    {90, 250},  // Rule 74: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Basah1, Kelembapan2 is Kering2
    {25, 155},  // Rule 75: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Basah2
    {55, 195},  // Rule 76: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Lembab2
    {95, 255},  // Rule 77: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Lembab1, Kelembapan2 is Kering2
    {30, 160},  // Rule 78: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Basah2
    {60, 200},  // Rule 79: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Lembab2
    {100, 260}, // Rule 80: Suhu1 is Panas1, Suhu2 is Panas2, Kelembapan1 is Kering1, Kelembapan2 is Kering2
};

void fuzzify(float inputs[], float fuzzyValues[][NUM_SETS])
{
    for (int i = 0; i < NUM_INPUTS; i++)
    {
        for (int j = 0; j < NUM_SETS; j++)
        {
            fuzzyValues[i][j] = membershipFunction(inputs[i], fuzzySets[i][j][0], fuzzySets[i][j][1], fuzzySets[i][j][2]);
        }
    }
}

void evaluateRules(float fuzzyValues[][NUM_SETS], float outputs[])
{
    float ruleWeights[NUM_RULES];
    for (int i = 0; i < NUM_RULES; i++)
    {
        ruleWeights[i] = 1.0;
    }

    for (int i = 0; i < NUM_RULES; i++)
    {
        int indices[NUM_INPUTS];
        int temp = i;
        for (int j = 0; j < NUM_INPUTS; j++)
        {
            indices[j] = temp % NUM_SETS;
            temp /= NUM_SETS;
        }

        float ruleWeight = 1.0;
        for (int j = 0; j < NUM_INPUTS; j++)
        {
            ruleWeight *= fuzzyValues[j][indices[j]];
        }
        ruleWeights[i] = ruleWeight;
    }

    float numerator[2] = {0.0, 0.0};
    float denominator = 0.0;

    for (int i = 0; i < NUM_RULES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            numerator[j] += ruleWeights[i] * ruleConsequents[i][j];
        }
        denominator += ruleWeights[i];
    }

    for (int j = 0; j < 2; j++)
    {
        outputs[j] = numerator[j] / denominator;
    }
}

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(SECRET_SSID);
        while (WiFi.status() != WL_CONNECTED)
        {
            WiFi.begin(ssid, pass);
        }
        Serial.print(".");
        delay(5000);
        Serial.println("\nConnected.");
    }

    ThingSpeak.begin(client);

    dht1.begin();
    dht2.begin();

    dimmer.begin(NORMAL_MODE, ON);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(ENA, ledChannel);

    lcd.init();
    lcd.clear();
    lcd.backlight();
}

void loop()
{
    if (millis() - cetakPerDetik >= 10000)
    {
        cetakPerDetik = millis();

        // thingspeak set fields
        ThingSpeak.setField(1, in_temp1);
        ThingSpeak.setField(2, in_hum1);
        ThingSpeak.setField(3, in_temp2);
        ThingSpeak.setField(4, in_hum2);
        ThingSpeak.setField(5, out_heater);
        ThingSpeak.setField(6, out_fan);

        // dht
        in_temp1 = dht1.readTemperature();
        in_temp2 = dht2.readTemperature();
        in_hum1 = dht1.readHumidity();
        in_hum2 = dht2.readHumidity();

        // fuzzy
        float inputs[NUM_INPUTS] = {in_temp1, in_temp2, in_hum1, in_hum2};

        float fuzzyValues[NUM_INPUTS][NUM_SETS];
        fuzzify(inputs, fuzzyValues);

        float outputs[2];
        evaluateRules(fuzzyValues, outputs);

        // assign output to var
        out_heater = outputs[0];
        out_fan = outputs[1];

        // write to thingspeak
        int x = ThingSpeak.writeFields(channel_ID, apiKey);
        if (x == 200)
        {
            Serial.println("Channel updated.");
        }
        else
        {
            Serial.println("Problem updating channel. HTTP error code " + String(x));
        }

        // dimmer
        dimmer.setPower(out_heater);

        // fan
        ledcWrite(ledChannel, out_fan);
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);

        // lcd
        lcd.setCursor(0, 0);
        lcd.print("T1:");
        lcd.print(in_temp1, 1);
        lcd.setCursor(7, 0);
        lcd.print(" H1:");
        lcd.print(in_hum1, 1);
        lcd.setCursor(0, 1);
        lcd.print("HE:");
        lcd.print(out_heater, 1);
        lcd.setCursor(8, 1);
        lcd.print("FA:");
        lcd.print(out_fan, 1);

        // print
        Serial.print("suhu1: ");
        Serial.print(in_temp1);
        Serial.print(" | suhu2: ");
        Serial.print(in_temp2);
        Serial.print(" | hum1: ");
        Serial.print(in_hum1);
        Serial.print(" | hum2: ");
        Serial.print(in_hum2);
        Serial.print(" | heater: ");
        Serial.print(out_heater);
        Serial.print(" | kipas: ");
        Serial.println(out_fan);
    }
}
