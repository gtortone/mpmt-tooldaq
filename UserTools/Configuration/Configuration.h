#ifndef Configuration_H
#define Configuration_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

class Configuration: public Tool {

public:

   Configuration();
   bool Initialise(std::string configfile,DataModel &data);
   bool Execute();
   bool Finalise();

private:

   std::string m_configfile;
   bool LoadConfig();  
};


#endif
