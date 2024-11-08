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
      hvdev = new HVDevice(port);
   } catch (std::exception &e) {
      *m_log << ML(1) << e.what() << std::endl;
      return false;
   }

   hvdev->Probe();

   for(uint8_t ch : hvdev->GetChannelList()) {
      if(m_verbose > 1)
         *m_log << ML(3) << "HV module " << ch << " detected" << std::endl;
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
            hvdev->Set(id, value, std::string(label));
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

   delete hvdev;

   return true;
}

void BoardControl::Thread(Thread_args* arg) {

   BoardControl_args* args=reinterpret_cast<BoardControl_args*>(arg);

   args->bd->lapse = args->bd->period - (boost::posix_time::microsec_clock::universal_time() - args->bd->last);

   if(args->bd->lapse.is_negative() && args->bd->m_data->services) {

      for(int mod : args->bd->hvdev->GetChannelList()) {

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
         hvdev->FindChannel(id);
      } catch (std::exception &e) {
         return e.what();
      }
      hvdev->Reset(id);
   }

   return "ok";
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
         hvdev->Set(id, value, label);
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

   tmp.Set("status", hvdev->GetPowerStatus(id));
   tmp.Set("voltage", hvdev->GetVoltageLevel(id));
   tmp.Set("current", hvdev->GetCurrent(id));
   tmp.Set("temperature", hvdev->GetTemperature(id));
   tmp.Set("alarm", hvdev->GetAlarm(id));
   tmp >> str;

   return str; 
}
