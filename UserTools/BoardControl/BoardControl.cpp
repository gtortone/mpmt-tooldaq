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
   m_configfile = configfile;
   InitialiseConfiguration(configfile);
   
   LoadConfig();

   last = boost::posix_time::microsec_clock::universal_time();
   
   m_util=new Utilities();
   args=new BoardControl_args();

   ctx = modbus_new_rtu(port.c_str(), 115200, 'N', 8, 1);

   if(ctx == NULL) {
      const char *msg = "unable to set modbus context";
      if(m_data->services)
         m_data->services->SendLog(msg, 2);
      *m_log << ML(0) << msg << std::endl;
      return false;
   }

   if(modbus_connect(ctx) == -1) {
      const char *msg = "unable to connect to modbus device";
      if(m_data->services)
         m_data->services->SendLog(msg, 2);
      *m_log << ML(0) << msg << std::endl;
      return false;
   }

   HVProbe();

   // configure tool using data retrieved by config file
   Configure(m_variables);

   m_data->sc_vars.Add("hv-reset", COMMAND, [this](const char* value) -> std::string { return HVResetFromCommand(value); } );
   m_data->sc_vars.Add("hv-power", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "power"); } );
   m_data->sc_vars.Add("hv-set-voltage", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "voltage"); } );
   m_data->sc_vars.Add("hv-set-rampup-rate", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "rampup_rate"); } );
   m_data->sc_vars.Add("hv-set-rampdown-rate", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "rampdown_rate"); } );
   m_data->sc_vars.Add("hv-set-voltage-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "voltage_limit"); } );
   m_data->sc_vars.Add("hv-set-current-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "current_limit"); } );
   m_data->sc_vars.Add("hv-set-temperature-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "temperature_limit"); } );
   m_data->sc_vars.Add("hv-set-triptime-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "triptime_limit"); } );
   m_data->sc_vars.Add("hv-set-threshold", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "threshold"); } );
   m_data->sc_vars.Add("hv-get-status", COMMAND, [this](const char *value) -> std::string { return HVStatusFromCommand(value); } );
   
   args->bd = this;
   m_util->CreateThread("hv", &Thread, args);
   
   ExportConfiguration();

   //m_variables.Print();
   
   return true;
}


bool BoardControl::Execute() {

   if(m_data->change_config){
      InitialiseConfiguration(m_configfile);
      LoadConfig();
      // configure tool using JSON data retrieved from DB
      Configure(m_data->vars);
      ExportConfiguration();
   }

   return true;
}

bool BoardControl::LoadConfig() {

   if(!m_variables.Get("verbose", m_verbose))
      m_verbose=1;

   if(!m_variables.Get("port", port))
      port = "/dev/ttyPS2";

   uint16_t period_sec;
   if(!m_variables.Get("hv_period_sec", period_sec))
      period_sec = 30;
   period = boost::posix_time::seconds(period_sec);
   
   return true;
}

void BoardControl::Configure(Store s) {

   std::vector<std::string> vars = s.Keys();
   for(std::string var: vars) {
      int id;
      int value;
      char label[32];
   
      if(sscanf(var.c_str(), "hv_%d_%s", &id, label) == 2) {
         s.Get<int>(var, value);
         try {
            HVSet(id, value, std::string(label));
         } catch (std::exception &e) {
            if(m_verbose > 1)   
               *m_log << ML(0) << "id: " << id << " value: " << value << " label: " << label << " => " << e.what() << std::endl;
         }
      }
   }

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

   args->bd->lapse = args->bd->period - (boost::posix_time::microsec_clock::universal_time() - args->bd->last);

   if(args->bd->lapse.is_negative()) {

      for(int mod : args->bd->modList) {
         // send monitoring data
         std::stringstream s;
         std::string json_str;
         s << "HV-" << args->bd->m_data->services->GetDeviceName() << args->bd->m_data->mpmt_id << "-" << mod;
         json_str = args->bd->HVGetMonitoringInfo(mod);
         
         args->bd->m_data->services->SendMonitoringData(json_str, s.str());

         if(args->bd->m_verbose > 1)
            *(args->bd->m_log) << args->bd->ML(0) << s.str() << ": " << json_str << std::endl;
      }

      args->bd->last = boost::posix_time::microsec_clock::universal_time();
   }

   usleep(5000);
}

void BoardControl::HVProbe(void) {

   uint16_t test;

   for(int i=1; i<=20; i++) {
      modbus_set_slave(ctx, i);
      if(modbus_read_registers(ctx, 0, 1, &test) == -1)
         continue;
      if(m_verbose > 1)
         *m_log << ML(3) << "HV module " << i << " detected" << std::endl;
      modList.push_back(i);
   }
}

bool BoardControl::HVFindModule(uint8_t id) {

   if(std::find(modList.begin(), modList.end(), id) == modList.end())
      throw(HVModuleNotFound("HV module not found"));

   return true;
}

// HV reset

std::string BoardControl::HVResetFromCommand(const char *cmd) {

   std::string params;
   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::Reset params: " << params.c_str() << std::endl;

   int id;
   
   if(sscanf(params.c_str(), "%d", &id) != 1) {

      return "HV command parse error";

   } else {
     
      try {
         HVFindModule(id);
      } catch (std::exception &e) {
         return e.what();
      }
      HVReset(id);
   }

   return "ok";
}

void BoardControl::HVReset(uint8_t id) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 2, true);
   mbus.unlock();
}

void BoardControl::HVSet(uint8_t id, int value, std::string label) {

   HVFindModule(id);

   if(std::find(HVparam.begin(), HVparam.end(), label) == HVparam.end())
      throw(HVParamNotFound("HV parameter not found"));

   if(label == "power")
      HVSetPower(id, value);
   else if(label == "voltage_level")
      HVSetVoltageLevel(id, value);
   else if(label == "rampup_rate")
      HVSetRampupRate(id, value);
   else if(label == "rampdown_rate")
      HVSetRampdownRate(id, value);
   else if(label == "voltage_limit")
      HVSetVoltageLimit(id, value);
   else if(label == "current_limit")
      HVSetCurrentLimit(id, value);
   else if(label == "temperature_limit")
      HVSetTemperatureLimit(id, value);
   else if(label == "triptime_limit")
      HVSetTriptimeLimit(id, value);
   else if(label == "threshold")
      HVSetThreshold(id, value);
}

// HV set from SlowControl command

std::string BoardControl::HVSetFromCommand(const char *cmd, std::string label) {

   int id, value;
   std::string params;

   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::" << label.c_str() << " params: " << params.c_str() << std::endl;

   if(sscanf(params.c_str(), "%d,%d", &id, &value) != 2) {

      return "HV command parse error";

   } else {

      try {
         HVSet(id, value, label);
      } catch (std::exception &e) {
         return e.what();
      }
   }

   return "ok";
}

// HV status from SlowControl command

std::string BoardControl::HVStatusFromCommand(const char *cmd) {

   std::string params;
   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::Status params: " << params.c_str() << std::endl;

   int id;
   
   if(sscanf(params.c_str(), "%d", &id) != 1) {

      return "HV command parse error";

   } else {

      try {
         HVFindModule(id);
      } catch (std::exception &e) {
         return e.what();
      }
      return HVGetMonitoringInfo(id);
   }
}

void BoardControl::HVSetPower(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 1, value!=0);
   mbus.unlock();
}

void BoardControl::HVSetVoltageLevel(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x26, value);
   mbus.unlock();
}

void BoardControl::HVSetRampupRate(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x23, value);
   mbus.unlock();
}

void BoardControl::HVSetRampdownRate(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x24, value);
   mbus.unlock();
}

void BoardControl::HVSetVoltageLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x27, value);
   mbus.unlock();
}

void BoardControl::HVSetCurrentLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x25, value);
   mbus.unlock();
}

void BoardControl::HVSetTemperatureLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x2F, value);
   mbus.unlock();
}

void BoardControl::HVSetTriptimeLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x22, value);
   mbus.unlock();
}

void BoardControl::HVSetThreshold(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x2D, value);
   mbus.unlock();
}

int BoardControl::HVGetPowerStatus(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x06, 1, &value);
   mbus.unlock();
   return value;
}

double BoardControl::HVGetVoltageLevel(uint8_t id) {

   uint16_t msb, lsb;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2A, 1, &lsb);
      modbus_read_registers(ctx, 0x2B, 1, &msb);
   mbus.unlock();

   return ((msb << 16) + lsb) / 1000.0;
}

double BoardControl::HVGetCurrent(uint8_t id) {

   uint16_t msb, lsb;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x28, 1, &lsb);
      modbus_read_registers(ctx, 0x29, 1, &msb);
   mbus.unlock();

   return ((msb << 16) + lsb) / 1000.0;
}

int BoardControl::HVGetTemperature(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x07, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetRampupRate(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x23, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetRampdownRate(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x24, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetVoltageLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x27, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetCurrentLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x25, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetTemperatureLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2F, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetTriptimeLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x22, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetThreshold(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2D, 1, &value);
   mbus.unlock();

   return value;
}

int BoardControl::HVGetAlarm(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2E, 1, &value);
   mbus.unlock();

   return value;
}

std::string BoardControl::HVGetMonitoringInfo(uint8_t id) {

   Store tmp;
   std::string str; 

   tmp.Set("status", HVGetPowerStatus(id));
   tmp.Set("voltage", HVGetVoltageLevel(id));
   tmp.Set("current", HVGetCurrent(id));
   tmp.Set("temperature", HVGetTemperature(id));
   tmp.Set("alarm", HVGetAlarm(id));
   tmp >> str;

   return str; 
}
