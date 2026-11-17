// ============================================================

// Human Presence Detection System

// Sensors: MLX90614 + MQ-135 + MPU6050

// Board: Arduino UNO

// Author: Generated for your wiring diagram

// ============================================================



#include <Wire.h>

#include <Adafruit_MLX90614.h>



// ─── Pin Definitions ────────────────────────────────────────

#define MQ135_PIN A0 // MQ-135 analog output -> A0

// SDA -> A4 (shared I2C bus: MLX90614 + MPU6050)

// SCL -> A5 (shared I2C bus: MLX90614 + MPU6050)



// ─── I2C Addresses ──────────────────────────────────────────

#define MPU6050_ADDR 0x68 // AD0 pin LOW (default)

#define MLX90614_ADDR 0x5A // Default address



// ─── Detection Thresholds (tuned for real-world conditions) ─

#define TEMP_MIN 30.0 // Min body temp (°C)

#define TEMP_MAX 40.0 // Max body temp (°C)

#define MQ135_THRESHOLD 300 // Raw ADC value above baseline

#define MOTION_THRESHOLD 800 // Accel magnitude delta (raw)

#define VOTES_NEEDED 2 // Out of 3 sensors needed



// ─── Timing ─────────────────────────────────────────────────

#define SAMPLE_INTERVAL 500 // ms between readings

#define BASELINE_SAMPLES 30 // Samples to build MQ-135 baseline



// ─── Objects ────────────────────────────────────────────────

Adafruit_MLX90614 mlx = Adafruit_MLX90614();



// ─── Globals ────────────────────────────────────────────────

int mq135Baseline = 0;

int16_t lastAccelX = 0;

int16_t lastAccelY = 0;

int16_t lastAccelZ = 0;

bool mpuAvailable = false;

bool mlxAvailable = false;



// ============================================================

// MPU6050 Helper Functions (direct I2C, no extra library)

// ============================================================



void mpuWriteReg(uint8_t reg, uint8_t value) {

Wire.beginTransmission(MPU6050_ADDR);

Wire.write(reg);

Wire.write(value);

Wire.endTransmission(true);

}



bool mpuInit() {

// Wake up MPU6050 (clear sleep bit in PWR_MGMT_1 register 0x6B)

mpuWriteReg(0x6B, 0x00);

delay(100);



// Verify WHO_AM_I register (0x75) should return 0x68

Wire.beginTransmission(MPU6050_ADDR);

Wire.write(0x75);

Wire.endTransmission(false);

Wire.requestFrom(MPU6050_ADDR, 1, true);

uint8_t whoAmI = Wire.read();

return (whoAmI == 0x68);

}



void mpuReadAccel(int16_t &ax, int16_t &ay, int16_t &az) {

// Accel data starts at register 0x3B

Wire.beginTransmission(MPU6050_ADDR);

Wire.write(0x3B);

Wire.endTransmission(false);

Wire.requestFrom(MPU6050_ADDR, 6, true);



ax = (Wire.read() << 8) | Wire.read();

ay = (Wire.read() << 8) | Wire.read();

az = (Wire.read() << 8) | Wire.read();

}



// ============================================================

// Baseline Calibration for MQ-135

// ============================================================



void calibrateMQ135() {

Serial.println("[MQ-135] Calibrating baseline (keep area clear)...");

long sum = 0;

for (int i = 0; i < BASELINE_SAMPLES; i++) {

sum += analogRead(MQ135_PIN);

delay(200);

}

mq135Baseline = sum / BASELINE_SAMPLES;

Serial.print("[MQ-135] Baseline set to: ");

Serial.println(mq135Baseline);

}



// ============================================================

// Sensor Vote Functions

// Each returns: 1 = human signal, 0 = no signal

// ============================================================



int checkMLX90614(float &objTemp, float &ambTemp) {

if (!mlxAvailable) return 0;



ambTemp = mlx.readAmbientTempC();

objTemp = mlx.readObjectTempC();



Serial.print("[MLX90614] Ambient: ");

Serial.print(ambTemp, 1);

Serial.print(" C | Object: ");

Serial.print(objTemp, 1);

Serial.print(" C --> ");



if (objTemp >= TEMP_MIN && objTemp <= TEMP_MAX) {

Serial.println("VOTE: HUMAN");

return 1;

}

Serial.println("VOTE: none");

return 0;

}



int checkMQ135(int &rawValue, int &delta) {

rawValue = analogRead(MQ135_PIN);

delta = rawValue - mq135Baseline;





Serial.print("[MQ-135] Raw: ");

Serial.print(rawValue);

Serial.print(" | Delta: ");

Serial.print(delta);

Serial.print(" --> ");



if (delta > MQ135_THRESHOLD) {

Serial.println("VOTE: HUMAN");

return 1;

}

Serial.println("VOTE: none");

return 0;

}



int checkMPU6050(long &magnitude) {

if (!mpuAvailable) return 0;



int16_t ax, ay, az;

mpuReadAccel(ax, ay, az);



// Calculate change in acceleration vector magnitude

long dX = abs(ax - lastAccelX);

long dY = abs(ay - lastAccelY);

long dZ = abs(az - lastAccelZ);

magnitude = dX + dY + dZ;



lastAccelX = ax;

lastAccelY = ay;

lastAccelZ = az;



Serial.print("[MPU6050] Motion magnitude: ");

Serial.print(magnitude);

Serial.print(" --> ");



if (magnitude > MOTION_THRESHOLD) {

Serial.println("VOTE: HUMAN");

return 1;

}

Serial.println("VOTE: none");

return 0;

}



// ============================================================

// Setup

// ============================================================



void setup() {

Serial.begin(9600);

Wire.begin();



Serial.println("=========================================");

Serial.println(" Human Presence Detection System ");

Serial.println("=========================================");



// Initialize MLX90614

if (mlx.begin()) {

mlxAvailable = true;

Serial.println("[MLX90614] OK");

} else {

Serial.println("[MLX90614] NOT FOUND - check wiring!");

}



// Initialize MPU6050

if (mpuInit()) {

mpuAvailable = true;

// Seed initial accel values

mpuReadAccel(lastAccelX, lastAccelY, lastAccelZ);

Serial.println("[MPU6050] OK");

} else {

Serial.println("[MPU6050] NOT FOUND - check wiring!");

}



// MQ-135 warmup + calibration

Serial.println("[MQ-135] Warming up (10 seconds)...");

delay(10000); // MQ sensors need warmup time

calibrateMQ135();



Serial.println("=========================================");

Serial.println("System ready. Starting detection loop...");

Serial.println("=========================================\n");

}



// ============================================================

// Main Loop

// ============================================================



void loop() {

Serial.println("-----------------------------------------");



float objTemp = 0, ambTemp = 0;

int mqRaw = 0, mqDelta = 0;

long motion = 0;



// ── Collect votes from each sensor ──

int votes = 0;

votes += checkMLX90614(objTemp, ambTemp);

votes += checkMQ135(mqRaw, mqDelta);

votes += checkMPU6050(motion);



// ── Fusion Decision ──

Serial.print("\n>>> TOTAL VOTES: ");

Serial.print(votes);

Serial.print("/3 --> ");



if (votes >= VOTES_NEEDED) {

Serial.println("*** HUMAN PRESENT ***");

} else {

Serial.println(" No human detected");

}



Serial.println("-----------------------------------------\n");

delay(SAMPLE_INTERVAL);

}