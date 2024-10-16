#ifndef NetTransmit_H
#define NetTransmit_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

struct NetTransmit_args:Thread_args{

   NetTransmit_args();
   ~NetTransmit_args();

   DataModel* m_data;
   DAQUtilities* utils;
   
   int32_t *buffer_id;
   bool *next_xfer;
   int verbose;
};


class NetTransmit: public Tool {

public:

   NetTransmit();
   bool Initialise(std::string configfile,DataModel &data);
   
   bool Execute();
   bool Finalise();

private:

   int32_t buffer_id;
   bool next_xfer;

   static void Thread(Thread_args* arg);
   Utilities* m_util; 
   NetTransmit_args* args;
};


#endif
