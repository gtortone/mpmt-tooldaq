#include "Configuration.h"

Configuration::Configuration():Tool() {
}

bool Configuration::Initialise(std::string configfile, DataModel &data){

   m_configfile = configfile;
  
   InitialiseTool(data);
   InitialiseConfiguration(configfile);
   
   ExportConfiguration();

   if(!m_variables.Get("verbose",m_verbose))
      m_verbose = 1;

   m_data->change_config=false;
   m_data->load_config=false;
   //m_data->run_configuration=0;  // test run configuration

   return true;
}


bool Configuration::Execute() {

   if(m_data->change_config)
      m_data->change_config=false;
  
   if(m_data->load_config) {

      if(!LoadConfig())
         return false;
    
      m_data->load_config=false;
      m_data->change_config=true;
   }

   return true;
}


bool Configuration::Finalise() {

   return true;
}

bool Configuration::LoadConfig() {
  
   std::string config_json="";
   std::stringstream device;

   device << m_data->services->GetDeviceName() << m_data->mpmt_id;

   printf("Configuration:LoadConfig get configuration for device %s\n", device.str().c_str());

   *m_log << ML(0) << m_tool_name << "::LoadConfig get configuration for device " << device.str() << std::endl;
   
   if(m_data->services->GetRunDeviceConfig(config_json, m_data->run_configuration, device.str())) {
    
      //printf("config_json: %s\n", config_json.c_str());
      
      InitialiseConfiguration(m_configfile);
      m_data->vars.JsonParser(config_json);
      m_data->vars.Print();

      if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
      ExportConfiguration();
      
      //
      //*m_log << ML(0) << m_tool_name << "::LoadConfig got updated m_variables:\n======" << std::endl;
      //m_variables.Print();
      //
    
  } else {
    
      m_data->services->SendLog("ERROR "+m_tool_name+": Failed to load config from DB with error '"+config_json+"'" , v_error);
      //bool ok = m_data->services->SendAlarm("ERROR "+m_tool_name+": Failed to load config from DB with error '"+config_json+"'");

      return false;
  }
  
  return true;
}
