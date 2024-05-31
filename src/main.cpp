#include <Arduino.h>

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

// Example rule consequents array
float ruleConsequents[NUM_RULES][2] = {
    {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}, {20, 150}, {50, 190}, {90, 250}};

float inputs[NUM_INPUTS] = {30, 32, 75, 80};

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
    float fuzzyValues[NUM_INPUTS][NUM_SETS];
    fuzzify(inputs, fuzzyValues);

    float outputs[2];
    evaluateRules(fuzzyValues, outputs);

    Serial.print("Heating Lamp Output: ");
    Serial.println(outputs[0]);
    Serial.print("Kipas DC Output: ");
    Serial.println(outputs[1]);
}

void loop()
{
    // Put your main code here, to run repeatedly
}
