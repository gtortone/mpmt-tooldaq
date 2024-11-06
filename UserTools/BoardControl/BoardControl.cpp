#include "BoardControl.h"
#include <sstream>

BoardControl_args::BoardControl_args():Thread_args() {
}

BoardControl_args::~BoardControl_args() {
}


BoardControl::BoardControl():Tool() { 
}

bool BoardControl::Initialise(std::string configfile, DataModel &data){

   InitialiseTool(data);
   InitialiseConfiguration(configfile);
   
   if(!m_variables.Get("verbose",m_verbose))
      m_verbose=1;

   if(!m_variables.Get("port",port))
      port = "/dev/ttyPS2";
   
   m_util=new Utilities();
   args=new BoardControl_args();

   ctx = modbus_new_rtu(port.c_str(), 115200, 'N', 8, 1);

   if(ctx == NULL) {
      const char *msg = "unable to set modbus context";
      if(m_data->services)
         m_data->services->SendLog(msg, 2);
      printf("%s\n", msg);
      return false;
   }

   if(modbus_connect(ctx) == -1) {
      const char *msg = "unable to connect to modbus device";
      if(m_data->services)
         m_data->services->SendLog(msg, 2);
      printf("%s\n", msg);
      return false;
   }

   Probe();

   m_data->sc_vars.Add("hv-reset", COMMAND, [this](const char* value) -> std::string { return Reset(value); } );
   m_data->sc_vars.Add("hv-power", COMMAND, [this](const char* value) -> std::string { return Set(value, "POWER"); } );
   m_data->sc_vars.Add("hv-set-voltage", COMMAND, [this](const char* value) -> std::string { return Set(value, "VOLTAGE"); } );
   m_data->sc_vars.Add("hv-set-rampup-rate", COMMAND, [this](const char* value) -> std::string { return Set(value, "RAMPUP"); } );
   m_data->sc_vars.Add("hv-set-rampdown-rate", COMMAND, [this](const char* value) -> std::string { return Set(value, "RAMPDOWN"); } );
   m_data->sc_vars.Add("hv-set-voltage-limit", COMMAND, [this](const char* value) -> std::string { return Set(value, "VLIMIT"); } );
   m_data->sc_vars.Add("hv-set-current-limit", COMMAND, [this](const char* value) -> std::string { return Set(value, "ILIMIT"); } );
   m_data->sc_vars.Add("hv-set-temperature-limit", COMMAND, [this](const char* value) -> std::string { return Set(value, "TLIMIT"); } );
   m_data->sc_vars.Add("hv-set-triptime-limit", COMMAND, [this](const char* value) -> std::string { return Set(value, "TTLIMIT"); } );
   m_data->sc_vars.Add("hv-set-threshold", COMMAND, [this](const char* value) -> std::string { return Set(value, "THRESHOLD"); } );
   
   m_util->CreateThread("hv", &Thread, args);
   
   ExportConfiguration();
   
   return true;
}


bool BoardControl::Execute() {

  return true;
}


bool BoardControl::Finalise() {

   m_util->KillThread(args);
   
   delete args;
   args=0;
   
   delete m_util;
   m_util=0;

   modList.clear();
   
   modbus_close(ctx);
   modbus_free(ctx);
   
   return true;
}

void BoardControl::Thread(Thread_args* arg) {

   BoardControl_args* args=reinterpret_cast<BoardControl_args*>(arg);

}

void BoardControl::Probe(void) {

   uint16_t test;

   for(int i=1; i<=20; i++) {
      modbus_set_slave(ctx, i);
      if(modbus_read_registers(ctx, 0, 1, &test) == -1)
         continue;
      if(m_verbose > 1)
         printf("HV module %d detected\n", i);
      modList.push_back(i);
   }
}

std::string BoardControl::ParamCheck(std::string params, int &id, int &value) {

   if(sscanf(params.c_str(), "%d,%d", &id, &value) == 2) {
      // check id
      std::vector<uint8_t>::iterator it = std::find(modList.begin(), modList.end(), id);
      if(it == modList.end())
         return "HV module not present";
   } else return "wrong params (id, value)";

   return "ok";
}

// HV reset

std::string BoardControl::Reset(const char *cmd) {

   std::string params;
   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      printf("BoardControl::Reset params: %s\n", params.c_str());

   int id;
   
   if(sscanf(params.c_str(), "%d", &id) == 1) {

      // check id
      std::vector<uint8_t>::iterator it = std::find(modList.begin(), modList.end(), id);
      if(it == modList.end())
         return "HV module not present";

   } else return "wrong params (id)";

   return DoReset(id);
}

std::string BoardControl::DoReset(uint8_t id) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 2, true);
   mbus.unlock();

   return "ok";
}

// HV set

std::string BoardControl::Set(const char *cmd, std::string label) {

   std::string params;
   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      printf("BoardControl::%s params: %s\n", label.c_str(), params.c_str());

   int id, value;
   std::string msg = ParamCheck(params, id, value);

   if(msg == "ok") {
      if(label == "POWER")
         msg = SetPower(id, value);
      else if(label == "VOLTAGE")
         msg = SetVoltage(id, value);
   }

   return msg;
}

std::string BoardControl::SetPower(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 1, value!=0);
   mbus.unlock();

   return "ok";
}

std::string BoardControl::SetVoltage(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x26, value);
   mbus.unlock();

   return "ok";
}

#if 0
// HV set rate

std::string BoardControl::Rate(const char *cmd) {

}

// HV set limit

std::string BoardControl::Limit(const char *cmd) {

}

// HV set threshold

std::string BoardControl::Threshold(const char *cmd) {

}

#endif
