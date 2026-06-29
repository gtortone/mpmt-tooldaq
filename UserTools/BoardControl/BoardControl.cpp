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

   // configure tool using data retrieved by config file
   Configure(m_variables);

   m_data->sc_vars.Add("rc-read-register", COMMAND, [this](const char *value) -> std::string { return RCReadFromCommand(value); } );
   m_data->sc_vars.Add("rc-write-register", COMMAND, [this](const char *value) -> std::string { return RCWriteFromCommand(value); } );
   m_data->sc_vars.Add("rc-get-ratemeters", BUTTON, [this](const char *value) -> std::string { return RCGetRatemeters(); } );
   
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
   
      if(sscanf(var.c_str(), "rc_%d", &reg) == 1) {
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

   return true;
}

void BoardControl::Thread(Thread_args* arg) {

   BoardControl_args* args=reinterpret_cast<BoardControl_args*>(arg);

   args->bd->lapse = args->bd->period - (boost::posix_time::microsec_clock::universal_time() - args->bd->last);

   if(args->bd->lapse.is_negative() && args->bd->m_data->services) {

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

      args->bd->last = boost::posix_time::microsec_clock::universal_time();
   }

   usleep(5000);
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
