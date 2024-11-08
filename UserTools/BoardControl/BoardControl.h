#ifndef BoardControl_H
#define BoardControl_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

#include <vector>
#include <modbus/modbus.h>

#include "HVExceptions.h"

struct BoardControl;

struct BoardControl_args:Thread_args{

  BoardControl_args();
  ~BoardControl_args();

  BoardControl *bd;
};

class BoardControl: public Tool {

public:

   BoardControl();
   bool Initialise(std::string configfile,DataModel &data);
   bool LoadConfig();
   void Configure(Store s);
   bool Execute();
   bool Finalise();

private:

   modbus_t *ctx;
   std::string port;
   std::vector<uint8_t> modList;
   std::mutex mbus;
   std::string m_configfile;

   static void Thread(Thread_args* arg);
   Utilities* m_util;
   BoardControl_args* args;

   const std::vector<std::string> HVparam = {
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

   // HV monitoring timers
   boost::posix_time::time_duration period;
   boost::posix_time::time_duration lapse;
   boost::posix_time::ptime last;

   // HV methods
   void HVProbe(void);
   std::string HVSetFromCommand(const char *cmd, std::string label);
   std::string HVResetFromCommand(const char *cmd);
   std::string HVStatusFromCommand(const char *cmd);
   void HVSet(uint8_t id, int value, std::string label);
   std::string HVGetMonitoringInfo(uint8_t id);
   bool HVFindModule(uint8_t id);

   void HVReset(uint8_t id);
   void HVSetPower(uint8_t id, int value);
   void HVSetVoltageLevel(uint8_t id, int value);
   void HVSetRampupRate(uint8_t id, int value);
   void HVSetRampdownRate(uint8_t id, int value);
   void HVSetVoltageLimit(uint8_t id, int value);
   void HVSetCurrentLimit(uint8_t id, int value);
   void HVSetTemperatureLimit(uint8_t id, int value);
   void HVSetTriptimeLimit(uint8_t id, int value);
   void HVSetThreshold(uint8_t id, int value);

   int HVGetPowerStatus(uint8_t id);
   double HVGetVoltageLevel(uint8_t id);
   double HVGetCurrent(uint8_t id);
   int HVGetTemperature(uint8_t id);
   int HVGetRampupRate(uint8_t id);
   int HVGetRampdownRate(uint8_t id);
   int HVGetVoltageLimit(uint8_t id);
   int HVGetCurrentLimit(uint8_t id);
   int HVGetTemperatureLimit(uint8_t id);
   int HVGetTriptimeLimit(uint8_t id);
   int HVGetThreshold(uint8_t id);
   int HVGetAlarm(uint8_t id);

   // RC methods
   // ...
};


#endif
