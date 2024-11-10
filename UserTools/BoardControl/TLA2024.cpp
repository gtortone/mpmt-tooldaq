#include "TLA2024.h"
#include <stdexcept>

void TLA2024::writeRegister(uint8_t reg, uint16_t value) {

	unsigned char buf[3] = { reg, (uint8_t)(value >> 8) , (uint8_t)(value & 0xFF) };
	write(m_i2cHandle, buf, 3);
}

uint16_t TLA2024::readRegister(uint8_t reg) {

	unsigned char buf[1] = { reg };
	write(m_i2cHandle, buf, 1);

	unsigned char readbuf[2] = {  };
	read(m_i2cHandle, readbuf, 2);

	uint16_t registerValue = ((readbuf[0] << 8) | readbuf[1]);

	return registerValue;
}

TLA2024::TLA2024(const char* i2cDeviceName, uint8_t i2cAddress)
{
	m_i2cDeviceName = i2cDeviceName;
	m_i2cAddress = i2cAddress;
	m_conversionDelay = TLA2024_CONVERSIONDELAY;
	m_bitShift = 4;
	m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
	m_sps = SPS_1600;
	setConversionDelay();

	// Create the file descriptor for the i2c bus
	m_i2cHandle = open(m_i2cDeviceName, O_RDWR);

   if(m_i2cHandle == -1)
      throw(std::runtime_error("TLA2024 sensor file open error"));

	// Set the slave address
   if(ioctl(m_i2cHandle, I2C_SLAVE, m_i2cAddress) == -1)
      throw(std::runtime_error("TLA2024 set slave address error"));
}

void TLA2024::updateI2cDevice(const char* i2cDeviceName) {
	m_i2cDeviceName = i2cDeviceName;
}

void TLA2024::setGain(tlaGain_t gain) { 
   m_gain = gain;
}

tlaGain_t TLA2024::getGain() { 
   return m_gain;
}

void TLA2024::setSps(tlaSps_t sps) {
	m_sps = sps;
	setConversionDelay();
}

tlaSps_t TLA2024::getSps() {
	return m_sps;
}

uint16_t TLA2024::readADC_SingleEnded(uint8_t channel) {

	if (channel > 3) {
		return 0;
	}

   uint16_t config = 0;

   config |= TLA2024_REG_CONFIG_MODE_SINGLE;

	// Set PGA/voltage range
	config |= m_gain;

	// Set the sample rate
	config |= m_sps;

	// Set single-ended input channel
	switch (channel) {
	case (0):
		config |= TLA2024_REG_CONFIG_MUX_SINGLE_0;
		break;
	case (1):
		config |= TLA2024_REG_CONFIG_MUX_SINGLE_1;
		break;
	case (2):
		config |= TLA2024_REG_CONFIG_MUX_SINGLE_2;
		break;
	case (3):
		config |= TLA2024_REG_CONFIG_MUX_SINGLE_3;
		break;
	}

	// Set 'start single-conversion' bit
	config |= TLA2024_REG_CONFIG_OS_SINGLE;

   config |= TLA2024_REG_RESERVED;

	// Write config register to the ADC
	writeRegister(TLA2024_REG_POINTER_CONFIG, config);

	// Wait for the conversion to complete
	usleep(m_conversionDelay);
	do {
		usleep(10);
	} while (TLA2024_REG_CONFIG_OS_BUSY == (readRegister(TLA2024_REG_POINTER_CONFIG) & TLA2024_REG_CONFIG_OS_MASK));

	// Read the conversion results
	// Shift 12-bit results right 4 bits for the TLA2024
	return readRegister(TLA2024_REG_POINTER_CONVERT) >> m_bitShift;
}

int16_t TLA2024::readADC_Differential_0_1() {

   uint16_t config = 0;

   config |= TLA2024_REG_CONFIG_MODE_SINGLE;

	// Set PGA/voltage range
	config |= m_gain;

	// Set the sample rate
	config |= m_sps;

	// Set channels
	config |= TLA2024_REG_CONFIG_MUX_DIFF_0_1; // AIN0 = P, AIN1 = N

	// Set 'start single-conversion' bit
	config |= TLA2024_REG_CONFIG_OS_SINGLE;

   config |= TLA2024_REG_RESERVED;

	// Write config register to the ADC
	writeRegister(TLA2024_REG_POINTER_CONFIG, config);

	// Wait for the conversion to complete
	usleep(m_conversionDelay);
	do {
		usleep(10);
	} while (TLA2024_REG_CONFIG_OS_BUSY == (readRegister(TLA2024_REG_POINTER_CONFIG) & TLA2024_REG_CONFIG_OS_MASK));

	// Read the conversion results
	uint16_t res = readRegister(TLA2024_REG_POINTER_CONVERT) >> m_bitShift;

	if (m_bitShift == 0) {
		return (int16_t)res;
	}
	else {
		// Shift 12-bit results right 4 bits for the TLA2024,
		// making sure we keep the sign bit intact
		if (res > 0x07FF) {
			// negative number - extend the sign to 16th bit
			res |= 0xF000;
		}
		return (int16_t)res;
	}
}

int16_t TLA2024::readADC_Differential_2_3() {

   uint16_t config = 0;

   config |= TLA2024_REG_CONFIG_MODE_SINGLE;

	// Set PGA/voltage range
	config |= m_gain;

	// Set the sample rate
	config |= m_sps;

	// Set channels
	config |= TLA2024_REG_CONFIG_MUX_DIFF_2_3; // AIN2 = P, AIN3 = N

	// Set 'start single-conversion' bit
	config |= TLA2024_REG_CONFIG_OS_SINGLE;

   config |= TLA2024_REG_RESERVED;

	// Write config register to the ADC
	writeRegister(TLA2024_REG_POINTER_CONFIG, config);

	// Wait for the conversion to complete
	usleep(m_conversionDelay);
	do {
		usleep(10);           
	} while (TLA2024_REG_CONFIG_OS_BUSY == (readRegister(TLA2024_REG_POINTER_CONFIG) & TLA2024_REG_CONFIG_OS_MASK));

	// Read the conversion results
	uint16_t res = readRegister(TLA2024_REG_POINTER_CONVERT) >> m_bitShift;
	if (m_bitShift == 0) {
		return (int16_t)res;
	}
	else {
		// Shift 12-bit results right 4 bits for the TLA2024,
		// making sure we keep the sign bit intact
		if (res > 0x07FF) {
			// negative number - extend the sign to 16th bit
			res |= 0xF000;
		}
		return (int16_t)res;
	}
}

void TLA2024::setConversionDelay() {

   switch (m_sps) {

      case SPS_128:
         m_conversionDelay = 1000000 / 128;
         break;
      case SPS_250:
         m_conversionDelay = 1000000 / 250;
         break;
      case SPS_490:
         m_conversionDelay = 1000000 / 490;
         break;
      case SPS_920:
         m_conversionDelay = 1000000 / 920;
         break;
      case SPS_1600:
         m_conversionDelay = 1000000 / 1600;
         break;
      case SPS_2400:
         m_conversionDelay = 1000000 / 2400;
         break;
      case SPS_3300:
         m_conversionDelay = 1000000 / 3300;
         break;
      case SPS_860:
         m_conversionDelay = 1000000 / 3300;
         break;
      default:
         m_conversionDelay = 8000;
         break;
      }

   m_conversionDelay += 100; // Add 100 us to be safe
}
