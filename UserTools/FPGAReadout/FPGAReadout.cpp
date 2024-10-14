#include "FPGAReadout.h"
#include <sys/mman.h>
#include <sys/ioctl.h>

FPGAReadout::FPGAReadout():Tool() {

}

bool FPGAReadout::Initialise(std::string configfile, DataModel &data) {
  
   InitialiseTool(data);
   InitialiseConfiguration(configfile);

   if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
   if(!m_variables.Get("id",m_data->mpmt_id)) m_data->mpmt_id=1;

   fd = open("/dev/dma_proxy_rx", O_RDWR);
   if (fd < 1) {
      m_data->services->SendLog("unable to open DMA proxy device file", 2);\
      return false;
   }
   
   m_data->buf_ptr = (struct channel_buffer *) mmap(NULL, sizeof(struct channel_buffer) * RX_BUFFER_COUNT,
      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (m_data->buf_ptr == MAP_FAILED) {
      m_data->services->SendLog("failed to mmap rx channel", 2);
      return false;
   }

   memset(m_data->buf_ptr, 0, sizeof(struct channel_buffer));

   // before thread loop preload DMA transfer queue
   for(uint16_t i=0; i<RX_BUFFER_COUNT; i++)
      m_data->q1.push(i);

   // prepare for first transfer
   buffer_id = 0;
   next_xfer = true;

   ExportConfiguration();

   return true;
}


bool FPGAReadout::Execute() {

   std::cout << "in FPGAReadout::Execute()" << std::endl;

   if(next_xfer) {
      buffer_id = m_data->q1.pop();
      m_data->buf_ptr[buffer_id].length = 32768;
      ioctl(fd, START_XFER, &buffer_id);
      std::cout << "START_XFER DONE (" << buffer_id << ")" << std::endl;
   } 
   ioctl(fd, FINISH_XFER, &buffer_id);
   proxy_status status = m_data->buf_ptr[buffer_id].status;

   if(status == PROXY_NO_ERROR) {
      next_xfer = true;
      m_data->q2.push(buffer_id);
      std::cout << "PROXY_NO_ERROR (" << buffer_id << ") " << std::endl;
   } else next_xfer = false;

   return true;
}


bool FPGAReadout::Finalise() {

   munmap(m_data->buf_ptr, sizeof(struct channel_buffer));
   close(fd);
   
   m_data->q1.clear();
   m_data->q2.clear();
   m_data->q3.clear();

   buffer_id = 0;
   next_xfer = true;

   return true;
}
