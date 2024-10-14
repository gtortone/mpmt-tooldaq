#ifndef FPGAReadout_H
#define FPGAReadout_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

class FPGAReadout: public Tool {

public:

   FPGAReadout(); 
   bool Initialise(std::string configfile,DataModel &data); 
   bool Execute(); 
   bool Finalise(); 

private:

   int fd;
   int32_t buffer_id;
   bool next_xfer;

};

#endif
