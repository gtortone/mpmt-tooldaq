#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

#include <zmq.hpp>

#include "Store.h"
#include "BoostStore.h"
//#include "DAQLogging.h"
#include "DAQUtilities.h"
#include "SlowControlCollection.h"
#include "DAQDataModelBase.h"
#include "DAQHeader.h"

#include "dma-proxy.h"
#include "queue.hpp"

using namespace ToolFramework;

class DataModel : public DAQDataModelBase {
  
public:
  
   DataModel(); ///< Simple constructor 
   
   bool load_config;
   bool change_config;
   bool run_start;
   bool run_stop;
   bool sub_run;
   boost::posix_time::ptime start_time;
   
   unsigned long run_number;
   unsigned long sub_run_number;

   int mpmt_id;
   TSQueue<uint16_t> q1, q2, q3;
   struct channel_buffer *buf_ptr;   

};

#endif
