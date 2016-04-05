#include "rht03-humidity-temperature-sensor.h"

#include "application.h"

#define EXPECTED_TRANSITION_INTERRUPT_COUNT 84
#define PULSE_WIDTH_TRUE_THRESHOLD_MICROSECONDS 35

static void sensorInterruptHandlerRedirect();

enum {
	StateUnknown,
	StateIdle,
	StateListening
};



RHT03HumidityTemperatureSensor *RHT03HumidityTemperatureSensor::currentlyListeningInstance = NULL;


RHT03HumidityTemperatureSensor::RHT03HumidityTemperatureSensor(int aPin)
{
	pin = aPin;
	state = StateIdle;
}


double RHT03HumidityTemperatureSensor::getTemperature()
{
	return temperature;
}


double RHT03HumidityTemperatureSensor::getHumidity()
{
	return humidity;
}


void RHT03HumidityTemperatureSensor::update()
{
	while (1) {
		if (state == StateIdle) {
			delay(1000);

			// Pull the pin low for 10ms to request a measurement from the sensor
			pinMode(pin, OUTPUT);
			digitalWrite(pin, HIGH);
			delay(10);
			digitalWrite(pin, LOW);
			delay(10);
			digitalWrite(pin, HIGH);

			// Configure the pin for input and set up the change interrupt handler
			pinMode(pin, INPUT_PULLUP);
			listeningStartTime = micros();
			RHT03HumidityTemperatureSensor::currentlyListeningInstance = this;
			attachInterrupt(pin, sensorInterruptHandlerRedirect, CHANGE);

			state = StateListening;
		} else if (state == StateListening) {
			unsigned long listeningInterval = micros() - listeningStartTime;
			// We assume that the measurement is done after this interval
			if (interruptCount == EXPECTED_TRANSITION_INTERRUPT_COUNT || listeningInterval > 4000000) {
				// Collect the data
				unsigned int rh = payload[0] << 8 | payload[1];
				unsigned int t = payload[2] << 8 | payload[3];
				humidity = (double)rh/10;
				temperature = (double)t/10;

				// Detach interrupt handler and reset all state for the next measurement
				detachInterrupt(pin);
				lastInterruptTime = 0;
				listeningStartTime = 0;
				interruptCount = 0;
				for (int i = 0; i < PAYLOAD_BYTES; i++) {
					payload[i] = 0;
				}

				RHT03HumidityTemperatureSensor::currentlyListeningInstance = NULL;
				state = StateIdle;
				break;
			}
			delay(1000);
		}

	}
}



void RHT03HumidityTemperatureSensor::handleInterrupt()
{
	interruptCount++;

	unsigned long now = micros();
	unsigned long intervalSinceLastChange = 0;
	if (lastInterruptTime) {
		intervalSinceLastChange = now - lastInterruptTime;
	}
	lastInterruptTime = now;

	if (interruptCount < 4) {
		// The first three transitions are for the two leading 80us pulses, ignore those
		return;
	} else if (interruptCount % 2) {
		// Every even transition starting with the fourth is for the rising
		// edge that ends the 50us low period that comes before every bit.
		// We note the time of that even transition (see above) but ignore
		// it otherwise.
		// Every odd transition marks the end of a bit and the length
		// of the pulse will tell us if it was a 0 or 1 bit.
		int bitIndex = (interruptCount - 5) / 2;
		unsigned char value = intervalSinceLastChange > PULSE_WIDTH_TRUE_THRESHOLD_MICROSECONDS ? 1 : 0;
		// Store the bit into the appropriate bit position of the appropriate
		// payload byte.
		payload[bitIndex/8] |= (value << (7 - (bitIndex % 8)));
	}
}


void sensorInterruptHandlerRedirect() {
	RHT03HumidityTemperatureSensor::currentlyListeningInstance->handleInterrupt();
}




