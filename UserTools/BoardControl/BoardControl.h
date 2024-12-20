#ifndef BoardControl_H
#define BoardControl_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

#include "HVDevice.h"
#include "RCDevice.h"
#include "TLA2024.h"
#include "BME280.h"

struct BoardControl;

struct BoardControl_args:Thread_args{

  BoardControl_args();
  ~BoardControl_args();

  BoardControl *bd;
  HVDevice *hvmon;
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

   std::string m_configfile;
   std::string hv_port;
   std::string hv_busmode;
   HVDevice *hvdev;

   std::string rc_port; 
   RCDevice *rcdev;

   TLA2024 *tladev;
   BME280 *bmedev;

   static void Thread(Thread_args* arg);
   Utilities* m_util;
   BoardControl_args* args;

   // monitoring timers
   boost::posix_time::time_duration period;
   boost::posix_time::time_duration lapse;
   boost::posix_time::ptime last;

   // HV methods
   std::string HVSetFromCommand(const char *cmd, std::string label);
   std::string HVResetFromCommand(const char *cmd);
   std::string HVStatusFromCommand(const char *cmd);
   std::string HVGetMonitoringInfo(uint8_t id);

   // RC methods
   std::string RCReadFromCommand(const char *cmd);
   std::string RCWriteFromCommand(const char *cmd);
   std::string RCGetRatemeters(void);

   // sensors methods
   std::string SNGetPSStatus(void);
   std::string SNGetEnvStatus(void);
};

#endif
