#ifndef MQ135_WRAPPER_H
#define MQ135_WRAPPER_H

#include <Arduino.h>
#include <MQUnifiedsensor.h>

// Configuration defaults (override these before including this header if needed)
#ifndef MQ135_BOARD
#define MQ135_BOARD "Arduino MEGA 2560"
#endif

#ifndef MQ135_VOLTAGE_RESOLUTION
#define MQ135_VOLTAGE_RESOLUTION 5.0f
#endif

#ifndef MQ135_ADC_BIT_RESOLUTION
#define MQ135_ADC_BIT_RESOLUTION 10
#endif

#ifndef MQ135_ANALOG_PIN
#define MQ135_ANALOG_PIN A0
#endif

#ifndef MQ135_DIGITAL_PIN
#define MQ135_DIGITAL_PIN 3
#endif

#ifndef MQ135_RATIO_CLEAN_AIR
#define MQ135_RATIO_CLEAN_AIR 3.6f // Rs/R0 for clean air (typical)
#endif

// Load resistor value in kilo-ohms (most MQ-135 boards use 10k)
#ifndef MQ135_RL
#define MQ135_RL 10.0f
#endif

class MQ135Sensor {
private:
	MQUnifiedsensor mq;
	int digitalPin;
	bool calibrated;

public:
	MQ135Sensor()
		: mq(MQ135_BOARD, MQ135_VOLTAGE_RESOLUTION, MQ135_ADC_BIT_RESOLUTION, MQ135_ANALOG_PIN, "MQ-135"),
			digitalPin(MQ135_DIGITAL_PIN),
			calibrated(false) {}

	// Perform basic initialization and calibration. Place the sensor in clean air.
	void begin(unsigned long calibrationSamples = 10, unsigned long sampleIntervalMs = 100) {
		pinMode(digitalPin, INPUT);
		mq.setRegressionMethod(1); // Exponential curve
		mq.init();
		mq.setRL(MQ135_RL); // Set load resistor
		
		Serial.print(F("[MQ135] Config: Board=")); Serial.print(MQ135_BOARD);
		Serial.print(F(", V=")); Serial.print(MQ135_VOLTAGE_RESOLUTION);
		Serial.print(F("V, ADC=")); Serial.print(MQ135_ADC_BIT_RESOLUTION);
		Serial.print(F("bit, Pin=")); Serial.print(MQ135_ANALOG_PIN);
		Serial.print(F(", RL=")); Serial.print(MQ135_RL); Serial.println(F("kΩ"));

		float r0 = 0.0f;
		for (unsigned long i = 0; i < calibrationSamples; i++) {
			mq.update();
			r0 += mq.calibrate(MQ135_RATIO_CLEAN_AIR);
			delay(sampleIntervalMs);
		}
		r0 /= (float)calibrationSamples;
		mq.setR0(r0);
		calibrated = true;
		
		Serial.print(F("[MQ135] Calibration complete: R0=")); Serial.println(r0, 3);
	}

	// Update the sensor reading (reads the analog voltage internally)
	void update() {
		mq.update();
	}

	// Convenience getters for different target gases (ppm). (Phase 2)
	float readCO2() {
		// Direct calculation: ppm = A * (Rs/R0)^B
		// MQ-135 CO2 regression curve from datasheet
		const float A = 110.47f;
		const float B = -2.862f;
		
		float rs = mq.getRS();
		float r0 = mq.getR0();
		float ratio = rs / r0;
		
		// Calculate ppm using power formula
		float ppm = A * pow(ratio, B);
		return ppm;
	}

	float readNH3() {
		// Direct calculation for Ammonia
		const float A = 102.2f;
		const float B = -2.473f;
		float ratio = mq.getRS() / mq.getR0();
		return A * pow(ratio, B);
	}

	float readAlcohol() {
		// Direct calculation for Alcohol
		const float A = 77.255f;
		const float B = -3.18f;
		float ratio = mq.getRS() / mq.getR0();
		return A * pow(ratio, B);
	}

	float readCO() {
		// Direct calculation for Carbon Monoxide
		const float A = 605.18f;
		const float B = -3.937f;
		float ratio = mq.getRS() / mq.getR0();
		return A * pow(ratio, B);
	}

	float readToluene() {
		// Direct calculation for Toluene
		const float A = 44.947f;
		const float B = -3.445f;
		float ratio = mq.getRS() / mq.getR0();
		return A * pow(ratio, B);
	}

	float readAcetone() {
		// Direct calculation for Acetone
		const float A = 34.668f;
		const float B = -3.369f;
		float ratio = mq.getRS() / mq.getR0();
		return A * pow(ratio, B);
	}

	// Digital output from the module's onboard comparator (threshold adjustable via potentiometer)
	bool isAboveThreshold() const {
		return digitalRead(digitalPin) == HIGH;
	}

	bool isCalibrated() const { return calibrated; }
	float getR0() { return mq.getR0(); }
	
	// Diagnostic helpers
	int getRawADC() {
		return analogRead(MQ135_ANALOG_PIN);
	}
	
	float getVoltage() {
		int adc = getRawADC();
		return (adc / 4095.0) * MQ135_VOLTAGE_RESOLUTION;
	}
	
	void printDiagnostics() {
		int adc = getRawADC();
		float voltage = getVoltage();
		float r0 = getR0();
		float rs = mq.getRS();
		float ratio = rs / r0;
		
		Serial.print(F("[MQ135] ADC=")); Serial.print(adc);
		Serial.print(F(" V=")); Serial.print(voltage, 3);
		Serial.print(F("V R0=")); Serial.print(r0, 3);
		Serial.print(F(" Rs=")); Serial.print(rs, 3);
		Serial.print(F(" Rs/R0=")); Serial.println(ratio, 3);
	}

	// next phase
	int calculateOdorScore(int airQuality) {
		// MQ-135: Lower value = worse air quality
		// Convert 0-1023 → 0-100 score
		int aqScore = map(airQuality, 0, 1023, 0, 100);
		return aqScore;
	}

	// next phase
	String detectOdorType(int airquality, float humidity) {
		if (humidity > 80 && airquality < 300) return "LEMBAB";
		if (airquality < 200) return "ASAP";
		if (airquality > 800) return "FRESH";
		return "NORMAL";
	}
};

#endif // MQ135_WRAPPER_H

