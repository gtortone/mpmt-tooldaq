#include "FPGAReadout.h"
#include <sys/mman.h>
#include <sys/ioctl.h>

FPGAReadout_args::FPGAReadout_args():Thread_args() {
}

FPGAReadout_args::~FPGAReadout_args() {
}

FPGAReadout::FPGAReadout():Tool() {
}

bool FPGAReadout::Initialise(std::string configfile, DataModel &data) {
  
   InitialiseTool(data);
   InitialiseConfiguration(configfile);

   if(!m_variables.Get("verbose", m_verbose))
      m_verbose = 1;

   if(!m_variables.Get("id", m_data->mpmt_id))
      m_data->mpmt_id = 1;

   m_data->vars.Get<int>("Inline", _inline);

   m_util=new Utilities();

   fd = open("/dev/dma_proxy_rx", O_RDWR);
   if (fd < 1) {
      const char *msg = "unable to open DMA proxy device file";
      if(m_data->services)
         m_data->services->SendLog(msg, 2);
      *m_log << ML(0) << msg << std::endl;
      return false;
   }

   m_data->buf_ptr = (struct channel_buffer *) mmap(NULL, sizeof(struct channel_buffer) * RX_BUFFER_COUNT,
      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (m_data->buf_ptr == MAP_FAILED) {
      const char *msg = "failed to mmap rx channel";
      if(m_data->services)
         m_data->services->SendLog("failed to mmap rx channel", 2);
      *m_log << ML(0) << msg << std::endl;
      return false;
   }

   memset(m_data->buf_ptr, 0, sizeof(struct channel_buffer));

   // before thread loop preload DMA transfer queue
   for(uint16_t i=0; i<RX_BUFFER_COUNT; i++)
      m_data->q1.push(i);

   // prepare for first transfer
   buffer_id = 0;
   next_xfer = true;
   acquiring = false;

   args = new FPGAReadout_args();
   args->utils = new DAQUtilities(m_data->context);
   args->m_data = m_data;
   args->fd = fd;
   args->next_xfer = &next_xfer;
   args->buffer_id = &buffer_id;
   args->verbose = m_verbose;
   args->m_log = m_log;
   args->acquiring = &acquiring;

   m_util->CreateThread("readout", &Thread, args);

   ExportConfiguration();

   return true;
}

bool FPGAReadout::Execute() {

   if(_inline == -1) {
      if(m_data->run_start)
         acquiring = true;
      else if(m_data->run_stop)
         acquiring = false;
   } else acquiring = true;

   return true;
}

bool FPGAReadout::Finalise() {

   m_util->KillThread(args);

   delete args;
   args=0;

   delete m_util;
   m_util=0;

   munmap(m_data->buf_ptr, sizeof(struct channel_buffer));
   close(fd);
   
   m_data->q1.clear();
   m_data->q2.clear();
   m_data->q3.clear();

   buffer_id = 0;
   next_xfer = true;

   return true;
}

void FPGAReadout::Thread(Thread_args* arg) {

   FPGAReadout_args* args = reinterpret_cast<FPGAReadout_args*>(arg);

   ToolFramework::MsgL ml(3, args->verbose);

   if(!*(args->acquiring)) {
      usleep(100);
      return;
   }

   //if(args->_inline == -1) {
   //   // SlaveRunControl tool in execution...
   //   if(!args->m_data->acquiring) {
   //      usleep(100);
   //      return;
   //   }
   //}


   if(*(args->next_xfer)) {
      *(args->buffer_id) = args->m_data->q1.pop();
      args->m_data->buf_ptr[*(args->buffer_id)].length = 32768;
      ioctl(args->fd, START_XFER, args->buffer_id);
      if(args->verbose > 1) {
         std::stringstream msg;
         if(args->m_data->services) {
            msg << "FPGAReadout::Thread: START_XFER (" << *(args->buffer_id) << ")";
            args->m_data->services->SendLog(msg.str(), 3);
         }
         ToolFramework::MsgL ml(3, args->verbose);
         *(args->m_log) << ml << msg.str() << std::endl;
      } 
   } 
   ioctl(args->fd, FINISH_XFER, args->buffer_id);
   proxy_status status = args->m_data->buf_ptr[*(args->buffer_id)].status;

   if(status == PROXY_NO_ERROR) {
      *(args->next_xfer) = true;
      args->m_data->q2.push(*(args->buffer_id));
      if(args->verbose > 1) {
         std::stringstream msg;
         if(args->m_data->services) {
            msg << "FPGAReadout::Thread: PROXY_NO_ERROR(" << *(args->buffer_id) << ")";
            args->m_data->services->SendLog(msg.str(), 3);
         }
         ToolFramework::MsgL ml(3, args->verbose);
         *(args->m_log) << ml << msg.str() << std::endl;
      }
   } else *(args->next_xfer) = false;
}
