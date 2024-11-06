#ifndef FPGAReadout_H
#define FPGAReadout_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

struct FPGAReadout_args:Thread_args{

   FPGAReadout_args();
   ~FPGAReadout_args();
   
   DataModel* m_data;
   DAQUtilities* utils;
   ToolFramework::Logging *m_log;
   int fd;
   int32_t *buffer_id;
   bool *next_xfer;
   bool *acquiring;
   int verbose;
};

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
   int _inline;
   bool acquiring;

   static void Thread(Thread_args* arg);
   Utilities* m_util;
   FPGAReadout_args* args;
};

#endif
