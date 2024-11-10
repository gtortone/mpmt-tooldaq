#include <stdint.h>

class BME280 {

private:
   int m_i2cHandle;
   const char* m_i2cDeviceName;
   uint8_t m_i2cAddress;

   int digT[3], digH[6], digP[9];
   char data[8];
   float t_fine;
   
   void setup(void);
   void readCoeffs(void);
   void readData(void);
   
public:
   BME280(const char* i2cDeviceName, uint8_t i2cAddress);

   float readTemperature(void);
   float readHumidity(void);
   float readPressure(void);
};
