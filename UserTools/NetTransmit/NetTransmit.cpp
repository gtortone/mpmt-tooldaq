#include "NetTransmit.h"

NetTransmit::NetTransmit():Tool(){}


bool NetTransmit::Initialise(std::string configfile, DataModel &data){
  
  InitialiseTool(data);
  InitialiseConfiguration(configfile);
  //m_variables.Print();

  //your code here


  ExportConfiguration();

  return true;
}


bool NetTransmit::Execute(){

  return true;
}


bool NetTransmit::Finalise(){

  return true;
}
