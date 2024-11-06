#ifndef BoardControl_H
#define BoardControl_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

#include <vector>
#include <modbus/modbus.h>

struct BoardControl_args:Thread_args{

  BoardControl_args();
  ~BoardControl_args();

};

class BoardControl: public Tool {

public:

   BoardControl();
   bool Initialise(std::string configfile,DataModel &data);
   bool Execute();
   bool Finalise();

private:

   modbus_t *ctx;
   std::string port;
   std::vector<uint8_t> modList;
   std::mutex mbus;

   static void Thread(Thread_args* arg);
   Utilities* m_util;
   BoardControl_args* args;
   void Probe(void);
   std::string ParamCheck(std::string params, int &id, int &value);
   std::string Set(const char *cmd, std::string label);
   std::string Reset(const char *params);
   std::string DoReset(uint8_t id);
   //std::string Power(const char *params);
   std::string SetPower(uint8_t id, int value);
   //std::string Voltage(const char *params);
   std::string SetVoltage(uint8_t id, int value);
};


#endif
