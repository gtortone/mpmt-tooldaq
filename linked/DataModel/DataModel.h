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

using namespace ToolFramework;

/*o*
 * \class DataModel
 *
 * This class Is a transient data model class for your Tools within the ToolChain. If Tools need to comunicate they pass all data objects through the data model. There fore inter tool data objects should be deffined in this class. 
 *
 *
 * $Author: B.Richards $ 
 * $Date: 2019/05/26 18:34:00 $
 * Contact: b.richards@qmul.ac.uk
 *
 */

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
  
};

#endif
