#ifndef ALERT_SUB_H
#define ALERT_SUB_H

#include "Tool.h"
#include "Services.h"

class SlaveRunControl: public ToolFramework::Tool {
  public:
    SlaveRunControl(): m_context(1) {};

    bool Initialise(std::string configfile, DataModel&);
    bool Execute();
    bool Finalise() { return true; }

  private:
    zmq::context_t m_context;
    bool m_run_start;
    bool m_run_stop;
    //boost::posix_time::ptime m_start_time;
    unsigned long m_start_time;
};

#endif
