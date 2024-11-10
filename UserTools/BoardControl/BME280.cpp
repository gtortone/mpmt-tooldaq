#include "BME280.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdexcept>

BME280::BME280(const char* i2cDeviceName, uint8_t i2cAddress) {

   m_i2cDeviceName = i2cDeviceName;
	m_i2cAddress = i2cAddress;

	// Create the file descriptor for the i2c bus
	m_i2cHandle = open(m_i2cDeviceName, O_RDWR);

   if(m_i2cHandle == -1)
      throw(std::runtime_error("BME280 sensor file open error"));

	// Set the slave address
   if(ioctl(m_i2cHandle, I2C_SLAVE, m_i2cAddress) == -1)
      throw(std::runtime_error("BME280 set slave address error"));

   // setup device
   readCoeffs();
   setup();
}

void BME280::readCoeffs(void) {

   char buf[24] = {0};

   // Read 24 bytes of data from register(0x88)
   char reg[1] = {0x88};
   write(m_i2cHandle, reg, 1);
   read(m_i2cHandle, buf, 24);

   digT[0] = (buf[0] + buf[1] * 256);
   digT[1] = (buf[2] + buf[3] * 256);
   if(digT[1] > 32767)
      digT[1] -= 65536;

   digT[2] = (buf[4] + buf[5] * 256);
   if(digT[2] > 32767)
      digT[2] -= 65536;

   digP[0] = (buf[6] + buf[7] * 256);
   digP[1] = (buf[8] + buf[9] * 256);
   if(digP[1] > 32767)
      digP[1] -= 65536;

   digP[2] = (buf[10] + buf[11] * 256);
   if(digP[2] > 32767)
      digP[2] -= 65536;

   digP[3] = (buf[12] + buf[13] * 256);
   if(digP[3] > 32767)
      digP[3] -= 65536;

   digP[4] = (buf[14] + buf[15] * 256);
   if(digP[4] > 32767)
      digP[4] -= 65536;

   digP[5] = (buf[16] + buf[17] * 256);
   if(digP[5] > 32767)
      digP[5] -= 65536;

   digP[6] = (buf[18] + buf[19] * 256);
   if(digP[6] > 32767)
      digP[6] -= 65536;

   digP[7] = (buf[20] + buf[21] * 256);
   if(digP[7] > 32767)
      digP[7] -= 65536;

   digP[8] = (buf[22] + buf[23] * 256);
   if(digP[8] > 32767)
      digP[8] -= 65536;

   // Read 1 byte of data from register(0xA1)
   reg[0] = 0xA1;
   write(m_i2cHandle, reg, 1);
   read(m_i2cHandle, buf, 1);
   digH[0] = buf[0];

   // Read 7 bytes of data from register(0xE1)
   reg[0] = 0xE1;
   write(m_i2cHandle, reg, 1);
   read(m_i2cHandle, buf, 7);

   digH[1] = (buf[0] + buf[1] * 256);
   if(digH[1] > 32767)
      digH[1] -= 65536;

   digH[2] = buf[2] & 0xFF;

   digH[3] = (buf[3] * 16 + (buf[4] & 0xF));
   if(digH[3] > 32767)
      digH[3] -= 65536;

   digH[4] = (buf[4] / 16) + (buf[5] * 16);
   if(digH[4] > 32767)
      digH[4] -= 65536;

   digH[5] = buf[6];
   if(digH[5] > 127)
      digH[5] -= 256;
}

void BME280::setup(void) {

   // Select control humidity register(0xF2)
   // Humidity over sampling rate = 1(0x01)
   char config[2] = {0};
   config[0] = 0xF2;
   config[1] = 0x01;
   write(m_i2cHandle, config, 2);

   // Select control measurement register(0xF4)
   // Normal mode, temp and pressure over sampling rate = 1(0x27)
   config[0] = 0xF4;
   config[1] = 0x27;
   write(m_i2cHandle, config, 2);

   // Select config register(0xF5)
   // Stand_by time = 1000 ms(0xA0)
   config[0] = 0xF5;
   config[1] = 0xA0;
   write(m_i2cHandle, config, 2);
}

void BME280::readData(void) {

   // Read 8 bytes of data from register(0xF7)
   // pressure msb1, pressure msb, pressure lsb, temp msb1, temp msb, temp lsb, humidity lsb, humidity msb
   char reg[1] = {0xF7};
   write(m_i2cHandle, reg, 1); 
   read(m_i2cHandle, data, 8);
}

float BME280::readTemperature(void) {

   readData();
   // convert temperature data to 19-bits
   long adc_t = ((long)(data[3] * 65536 + ((long)(data[4] * 256) + (long)(data[5] & 0xF0)))) / 16;
   
   // temperature offset calculations
   float var1 = (((float)adc_t) / 16384.0 - ((float)digT[0]) / 1024.0) * ((float)digT[1]);
   float var2 = ((((float)adc_t) / 131072.0 - ((float)digT[0]) / 8192.0) *
      (((float)adc_t)/131072.0 - ((float)digT[0])/8192.0)) * ((float)digT[2]);

   // store t_fine for humidity and pressure reads 
   t_fine = (long)(var1 + var2);

   return ((var1 + var2) / 5120.0);

}

float BME280::readHumidity(void) {

   // read t_fine
   readTemperature();

   // convert humidity data
   long adc_h = (data[6] * 256 + data[7]);

   // humidity offset calculations
   float var_H = (((float)t_fine) - 76800.0);
   var_H = (adc_h - (digH[3] * 64.0 + digH[4] / 16384.0 * var_H)) * (digH[1] / 65536.0 * (1.0 + digH[5] / 67108864.0 * 
      var_H * (1.0 + digH[2] / 67108864.0 * var_H)));
   float humidity = var_H * (1.0 -  digH[0] * var_H / 524288.0);
   if(humidity > 100.0)
      humidity = 100.0;
   else if(humidity < 0.0)
      humidity = 0.0;

   return humidity;
}

float BME280::readPressure(void) {

   // read t_fine
   readTemperature();

   // convert pressure data to 19-bits
   long adc_p = ((long)(data[0] * 65536 + ((long)(data[1] * 256) + (long)(data[2] & 0xF0)))) / 16; 

   // Pressure offset calculations
   float var1 = ((float)t_fine / 2.0) - 64000.0;
   float var2 = var1 * var1 * ((float)digP[5]) / 32768.0;
   var2 = var2 + var1 * ((float)digP[4]) * 2.0;
   var2 = (var2 / 4.0) + (((float)digP[3]) * 65536.0);
   var1 = (((float) digP[2]) * var1 * var1 / 524288.0 + ((float) digP[1]) * var1) / 524288.0;
   var1 = (1.0 + var1 / 32768.0) * ((float)digP[0]);
   float p = 1048576.0 - (float)adc_p;
   p = (p - (var2 / 4096.0)) * 6250.0 / var1;
   var1 = ((float) digP[8]) * p * p / 2147483648.0;
   var2 = p * ((float) digP[7]) / 32768.0;

   return (p + (var1 + var2 + ((float)digP[6])) / 16.0) / 100.0;
}
