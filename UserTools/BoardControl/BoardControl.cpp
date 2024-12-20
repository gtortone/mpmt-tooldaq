#include "BoardControl.h"
#include <vector>
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

   try {
      rcdev = new RCDevice(rc_port);
   } catch (std::exception &e) {
      *m_log << ML(1) << e.what() << std::endl;
      return false;
   }

   try {
      hvdev = new HVDevice(hv_busmode, hv_port);
   } catch (std::exception &e) {
      *m_log << ML(1) << e.what() << std::endl;
      return false;
   }
   hvdev->Probe();

   for(uint8_t ch : hvdev->GetChannelList()) {
      if(m_verbose > 1)
         *m_log << ML(3) << "HV module " << ch << " detected" << std::endl;
   }

   try {
      tladev = new TLA2024("/dev/i2c-1", 0x48);
   } catch (std::exception &e) {
      *m_log << ML(1) << e.what() << std::endl;
      return false;
   }
   tladev->setSps(SPS_1600);

   try {
      bmedev = new BME280("/dev/i2c-1", 0x76);
   } catch (std::exception &e) {
      *m_log << ML(1) << e.what() << std::endl;
      return false;
   }

   // configure tool using data retrieved by config file
   Configure(m_variables);

   m_data->sc_vars.Add("hv-reset", COMMAND, [this](const char* value) -> std::string { return HVResetFromCommand(value); } );
   m_data->sc_vars.Add("hv-power", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "power"); } );
   m_data->sc_vars.Add("hv-set-voltage-level", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "voltage_level"); } );
   m_data->sc_vars.Add("hv-set-rampup-rate", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "rampup_rate"); } );
   m_data->sc_vars.Add("hv-set-rampdown-rate", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "rampdown_rate"); } );
   m_data->sc_vars.Add("hv-set-voltage-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "voltage_limit"); } );
   m_data->sc_vars.Add("hv-set-current-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "current_limit"); } );
   m_data->sc_vars.Add("hv-set-temperature-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "temperature_limit"); } );
   m_data->sc_vars.Add("hv-set-triptime-limit", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "triptime_limit"); } );
   m_data->sc_vars.Add("hv-set-threshold", COMMAND, [this](const char* value) -> std::string { return HVSetFromCommand(value, "threshold"); } );
   m_data->sc_vars.Add("hv-get-status", COMMAND, [this](const char *value) -> std::string { return HVStatusFromCommand(value); } );
   m_data->sc_vars.Add("rc-read-register", COMMAND, [this](const char *value) -> std::string { return RCReadFromCommand(value); } );
   m_data->sc_vars.Add("rc-write-register", COMMAND, [this](const char *value) -> std::string { return RCWriteFromCommand(value); } );
   m_data->sc_vars.Add("rc-get-ratemeters", BUTTON, [this](const char *value) -> std::string { return RCGetRatemeters(); } );
   m_data->sc_vars.Add("sn-read-ps", BUTTON, [this](const char *value) -> std::string { return SNGetPSStatus(); } );
   m_data->sc_vars.Add("sn-read-env", BUTTON, [this](const char *value) -> std::string { return SNGetEnvStatus(); } );
   
   args->bd = this;
   m_util->CreateThread("bc", &Thread, args);
   
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

   if(!m_variables.Get("hv_busmode", hv_busmode))
      hv_busmode = "tcp";

   if(!m_variables.Get("hv_port", hv_port))
      hv_port = "/dev/ttyPS2";

   if(!m_variables.Get("rc_port", rc_port))
      rc_port = "/dev/uio0";

   uint16_t period_sec;
   if(!m_variables.Get("monitoring_period_sec", period_sec))
      period_sec = 30;
   period = boost::posix_time::seconds(period_sec);
   
   return true;
}

void BoardControl::Configure(Store s) {

   std::vector<std::string> vars = s.Keys();
   for(std::string var: vars) {
      int id, reg;
      int value;
      uint32_t lvalue;
      std::string value_str;
      char label[32];
   
      if(sscanf(var.c_str(), "hv_%d_%s", &id, label) == 2) {
         s.Get<int>(var, value);
         try {
            hvdev->Set(id, value, std::string(label));
         } catch (std::exception &e) {
            if(m_verbose > 1)   
               *m_log << ML(0) << "(HV) id: " << id << " value: " << value << " label: " << label << " => " << e.what() << std::endl;
         }

      } else if(sscanf(var.c_str(), "rc_%d", &reg) == 1) {
         s.Get<std::string>(var, value_str);
         // check if value is in hex format
         size_t idx;
         lvalue = std::stol(value_str, &idx, 16);
         if(idx == value_str.length()) {
            rcdev->WriteRegister(reg, lvalue);
            if(m_verbose > 1)   
               *m_log << ML(0) << "(RC) reg: " << reg << " value: " << value_str << std::endl;
         } else {
            // check if value is in decimal format
            lvalue = std::stol(value_str, &idx, 10);
            if(idx == value_str.length()) {
               rcdev->WriteRegister(reg, lvalue);
               if(m_verbose > 1)   
                  *m_log << ML(0) << "(RC) reg: " << reg << " value: " << value_str << std::endl;
            }
         }
      }
   } // end for
}

bool BoardControl::Finalise() {

   m_util->KillThread(args);
   
   delete args;
   args=0;
   
   delete m_util;
   m_util=0;

   delete hvdev;

   return true;
}

void BoardControl::Thread(Thread_args* arg) {

   BoardControl_args* args=reinterpret_cast<BoardControl_args*>(arg);

   if(!args->hvmon) {
      args->hvmon = new HVDevice(args->bd->hv_busmode, args->bd->hv_port);
      args->hvmon->SetChannelList(args->bd->hvdev->GetChannelList());
   }

   args->bd->lapse = args->bd->period - (boost::posix_time::microsec_clock::universal_time() - args->bd->last);

   if(args->bd->lapse.is_negative() && args->bd->m_data->services) {

      // send HV monitoring data using channel list of hvdev
      for(int mod : args->bd->hvdev->GetChannelList()) {

         std::stringstream s;
         std::string json_str;

         s << "HV-" << args->bd->m_data->services->GetDeviceName() << args->bd->m_data->mpmt_id << "-" << mod;

         Store tmp;

         tmp.Set("status", args->hvmon->GetPowerStatus(mod));
         tmp.Set("voltage", args->hvmon->GetVoltageLevel(mod));
         tmp.Set("current", args->hvmon->GetCurrent(mod));
         tmp.Set("temperature", args->hvmon->GetTemperature(mod));
         tmp.Set("alarm", args->hvmon->GetAlarm(mod));
         tmp >> json_str;

         args->bd->m_data->services->SendMonitoringData(json_str, s.str());

         if(args->bd->m_verbose > 1)
            *(args->bd->m_log) << args->bd->ML(0) << s.str() << ": " << json_str << std::endl;
      }
   
      {
         // send RC monitoring data
         std::stringstream s;
         std::string json_str;

         s << "RC-" << args->bd->m_data->services->GetDeviceName() << args->bd->m_data->mpmt_id << "-rates";
         json_str = args->bd->RCGetRatemeters();

         args->bd->m_data->services->SendMonitoringData(json_str, s.str());

         if(args->bd->m_verbose > 1)
            *(args->bd->m_log) << args->bd->ML(0) << s.str() << ": " << json_str << std::endl;
      }

      {
         // send PS monitoring data
         std::stringstream s;
         std::string json_str;

         s << "SN-" << args->bd->m_data->services->GetDeviceName() << args->bd->m_data->mpmt_id << "-PS";
         json_str = args->bd->SNGetPSStatus();

         args->bd->m_data->services->SendMonitoringData(json_str, s.str());

         if(args->bd->m_verbose > 1)
            *(args->bd->m_log) << args->bd->ML(0) << s.str() << ": " << json_str << std::endl;
      }

      {
         // send environmental monitoring data
         std::stringstream s;
         std::string json_str;

         s << "SN-" << args->bd->m_data->services->GetDeviceName() << args->bd->m_data->mpmt_id << "-ENV";
         json_str = args->bd->SNGetEnvStatus();

         args->bd->m_data->services->SendMonitoringData(json_str, s.str());

         if(args->bd->m_verbose > 1)
            *(args->bd->m_log) << args->bd->ML(0) << s.str() << ": " << json_str << std::endl;
      }

      args->bd->last = boost::posix_time::microsec_clock::universal_time();
   }

   usleep(5000);
}

// HV slow control commands

std::string BoardControl::HVResetFromCommand(const char *cmd) {

   std::string params;
   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::Reset params: " << params.c_str() << std::endl;

   int id;

   HVDevice *hv;
   if(hv_busmode == "tcp") {
      hv = new HVDevice(hv_busmode, hv_port);
      hv->SetChannelList(hvdev->GetChannelList());
   } else hv = hvdev;
   
   if(sscanf(params.c_str(), "%d", &id) != 1) {

      return "HV command parse error";

   } else {
     
      try {
         hv->FindChannel(id);
      } catch (std::exception &e) {
         return e.what();
      }
      hv->Reset(id);
   }

   return "ok";
}

std::string BoardControl::HVSetFromCommand(const char *cmd, std::string label) {

   int id, value;
   std::string params;

   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::" << label.c_str() << " params: " << params.c_str() << std::endl;

   HVDevice *hv;
   if(hv_busmode == "tcp") {
      hv = new HVDevice(hv_busmode, hv_port);
      hv->SetChannelList(hvdev->GetChannelList());
   } else hv = hvdev;

   if(sscanf(params.c_str(), "%d,%d", &id, &value) != 2) {

      return "HV command parse error";

   } else {

      try {
         hv->Set(id, value, label);
      } catch (std::exception &e) {
         return e.what();
      }
   }

   return "ok";
}

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
         hvdev->FindChannel(id);
      } catch (std::exception &e) {
         return e.what();
      }
      return HVGetMonitoringInfo(id);
   }
}

std::string BoardControl::HVGetMonitoringInfo(uint8_t id) {

   Store tmp;
   std::string str; 

   HVDevice *hv;
   if(hv_busmode == "tcp") {
      hv = new HVDevice(hv_busmode, hv_port);
      hv->SetChannelList(hvdev->GetChannelList());
   } else hv = hvdev;

   tmp.Set("status", hv->GetPowerStatus(id));
   tmp.Set("voltage", hv->GetVoltageLevel(id));
   tmp.Set("current", hv->GetCurrent(id));
   tmp.Set("temperature", hv->GetTemperature(id));
   tmp.Set("alarm", hv->GetAlarm(id));
   tmp >> str;

   return str; 
}

// RC slow control commands

std::string BoardControl::RCReadFromCommand(const char *cmd) {

   uint16_t addr;
   std::string params;

   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::RCRead params: " << params.c_str() << std::endl;

   if(sscanf(params.c_str(), "%d", &addr) != 1) {

      return "RC command parse error";

   } else {

      uint32_t value = rcdev->ReadRegister(addr);

      Store tmp;
      std::string json_str;
      std::stringstream ss;

      ss << "\"0x" << std::hex << value << "\"";
      tmp.Set<uint32_t>("dec", value);
      tmp.Set<std::string>("hex", ss.str());

      tmp >> json_str;
   
      return json_str;
   }
}

std::string BoardControl::RCWriteFromCommand(const char *cmd) {

   uint16_t addr;
   uint32_t value;
   std::string params;

   m_data->sc_vars[cmd]->GetValue(params);

   if(m_verbose > 1)
      *m_log << ML(3) << "BoardControl::RCWrite params: " << params.c_str() << std::endl;

   if( (sscanf(params.c_str(), "%ld,0x%X", &addr, &value) == 2) || 
      (sscanf(params.c_str(), "%ld,%ld", &addr, &value) == 2) ) {

      rcdev->WriteRegister(addr, value);
      return "ok";

   }
   
   return "RC command parse error";
}

std::string BoardControl::RCGetRatemeters(void) {

   Store tmp;
   std::string str; 

   for(int i=1; i<=19; i++) {
      std::stringstream ss;
      ss << "rate_ch" << i;
      tmp.Set(ss.str(), rcdev->ReadRegister(7+i));
   }

   tmp >> str;

   return str; 
}

std::string BoardControl::SNGetPSStatus(void) {

   Store tmp;
   std::string str; 
   double v1, v2, i1, i2;

   tladev->setGain(GAIN_TWOTHIRDS);
   v1 = tladev->readADC_SingleEnded(0) * 3 * 3 / 1000.0;
   tmp.Set("+5.0V", v1);

   tladev->setGain(GAIN_TWO);
   i1 = tladev->readADC_SingleEnded(1) / 1000.0;
   tmp.Set("IpoeA", i1);

   tladev->setGain(GAIN_ONE);
   v2 = tladev->readADC_SingleEnded(2) * 2 * 2 / 1000.0;
   tmp.Set("+3.3V", v2);

   tladev->setGain(GAIN_TWO);
   i2 = tladev->readADC_SingleEnded(3) / 1000.0;
   tmp.Set("IpoeB", i2);

   tmp.Set("PpoeA", v1 * i1);
   tmp.Set("PpoeB", v1 * i2);

   tmp >> str;

   return str; 
}

std::string BoardControl::SNGetEnvStatus(void) {

   Store tmp;
   std::string str; 

   tmp.Set("board_temperature", bmedev->readTemperature());
   tmp.Set("board_humidity", bmedev->readHumidity());
   tmp.Set("board_pressure", bmedev->readPressure());

   tmp >> str;   

   return str; 
}
