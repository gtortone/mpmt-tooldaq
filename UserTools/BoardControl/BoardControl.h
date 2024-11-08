#ifndef BoardControl_H
#define BoardControl_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

#include "HVDevice.h"
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

   std::string port;
   std::string m_configfile;
   HVDevice *hvdev;

   static void Thread(Thread_args* arg);
   Utilities* m_util;
   BoardControl_args* args;

   // HV monitoring timers
   boost::posix_time::time_duration period;
   boost::posix_time::time_duration lapse;
   boost::posix_time::ptime last;

   // HV methods
   std::string HVSetFromCommand(const char *cmd, std::string label);
   std::string HVResetFromCommand(const char *cmd);
   std::string HVStatusFromCommand(const char *cmd);
   std::string HVGetMonitoringInfo(uint8_t id);

   // RC methods
   // ...
};


#endif
