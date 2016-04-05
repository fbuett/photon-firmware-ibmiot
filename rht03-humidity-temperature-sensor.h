
#define PAYLOAD_BITS 40
#define PAYLOAD_BYTES (PAYLOAD_BITS/8)

class RHT03HumidityTemperatureSensor {
    public:
        RHT03HumidityTemperatureSensor(int pin);
        double getTemperature();
        double getHumidity();
        void update();
        void handleInterrupt();
        static RHT03HumidityTemperatureSensor *currentlyListeningInstance;

    private:
        int pin;
        double temperature;
        double humidity;
        int state;
        unsigned long listeningStartTime = 0;
        volatile unsigned long lastInterruptTime = 0;
        volatile int interruptCount = 0;
        volatile unsigned char payload[PAYLOAD_BYTES];
};

