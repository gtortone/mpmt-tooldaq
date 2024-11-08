#ifndef HVDEVICE_H_
#define HVDEVICE_H_

#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <modbus/modbus.h>

class HVDevice {

private:
   modbus_t *ctx;
   std::string m_port;
   std::mutex mbus;
   std::vector<uint8_t> channelList;

   const std::vector<std::string> param = {
      "power",
      "voltage_level",
      "rampup_rate",
      "rampdown_rate",
      "voltage_limit",
      "current_limit",
      "temperature_limit",
      "triptime_limit",
      "threshold"
   };

public:
   HVDevice(std::string port);
   ~HVDevice();

   void Probe(void);
   std::vector<uint8_t> GetChannelList(void);
   bool FindChannel(uint8_t id);
   void Set(uint8_t id, int value, std::string label);

   void Reset(uint8_t id);
   void SetPower(uint8_t id, int value);
   void SetVoltageLevel(uint8_t id, int value);
   void SetRampupRate(uint8_t id, int value);
   void SetRampdownRate(uint8_t id, int value);
   void SetVoltageLimit(uint8_t id, int value);
   void SetCurrentLimit(uint8_t id, int value);
   void SetTemperatureLimit(uint8_t id, int value);
   void SetTriptimeLimit(uint8_t id, int value);
   void SetThreshold(uint8_t id, int value);

   int GetPowerStatus(uint8_t id);
   double GetVoltageLevel(uint8_t id); 
   double GetCurrent(uint8_t id);
   int GetTemperature(uint8_t id);
   int GetRampupRate(uint8_t id);
   int GetRampdownRate(uint8_t id);
   int GetVoltageLimit(uint8_t id);
   int GetCurrentLimit(uint8_t id);
   int GetTemperatureLimit(uint8_t id);
   int GetTriptimeLimit(uint8_t id);
   int GetThreshold(uint8_t id);
   int GetAlarm(uint8_t id);
};

#endif
