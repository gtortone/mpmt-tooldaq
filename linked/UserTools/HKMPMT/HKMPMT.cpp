#include "HKMPMT.h"

HKMPMT_args::HKMPMT_args():Thread_args() {
   data_sock = 0;
   utils = 0;
   m_data = 0;
}

HKMPMT_args::~HKMPMT_args() {
   delete data_sock;
   data_sock=0;
   delete utils;
   utils = 0;

   m_data = 0;
}


HKMPMT::HKMPMT():Tool() {
}


bool HKMPMT::Initialise(std::string configfile, DataModel &data) {

   InitialiseTool(data);
   InitialiseConfiguration(configfile);
   m_variables.Print();
   
   if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
   if(!m_variables.Get("id",m_id)) m_id=1;
   
   m_util=new Utilities();

   args = new HKMPMT_args();
   
   args->mpmt_id = m_id;
   args->m_data = m_data;
   args->data_sock = new zmq::socket_t(*(m_data->context), ZMQ_DEALER);
   args->data_port = "4444";
   args->data_sock->set(zmq::sockopt::routing_id, std::to_string(m_id));
   args->data_sock->bind("tcp://*:" + args->data_port);

   args->utils = new DAQUtilities(m_data->context);

   m_util->CreateThread("readout", &Thread, args);

   ExportConfiguration();
   
   return true;
}


bool HKMPMT::Execute() {

   return true;
}


bool HKMPMT::Finalise() {

   m_util->KillThread(args);
   
   delete args;
   args=0;
   
   delete m_util;
   m_util=0;
   
   return true;
}

void HKMPMT::Thread(Thread_args* arg){

   HKMPMT_args* args=reinterpret_cast<HKMPMT_args*>(arg);

   DAQHeader *dh = new DAQHeader();
   dh->SetCardType(2);
   dh->SetCardID(args->mpmt_id);

   // FIXME
   dh->SetCoarseCounter(800);

   /* 
      plain event (no subhits):
         0x8038118C
         0x0C076053
         0xFFFFF102

   uint16_t event[6] = { 0x3880, 0x8C11, 0x070C, 0x5360, 0xFFFF, 0x02F1 };

   */

   /*

      plain event (1 subhit):
         0x8038518C
         0x0C076053
         0x4C076053
         0xFFFFF102

   uint16_t event[8] = { 0x3880, 0x8C51, 0x070C, 0x5360, 0x074C, 0x5360, 0xFFFF, 0x02F1 };

   */

   /*

      plain event (2 subhits):
         0x8038918C
         0x0C076053
         0x4C076053
         0x4C076053
         0xFFFFF102

   uint16_t event[12] = { 0x3880, 0x8C91, 0x070C, 0x5360, 0x074C, 0x5360, 0x074C, 0x5360, 0xFFFF, 0x02F1 };

   */
   
   uint16_t event[12] = { 0x3880, 0x8C91, 0x070C, 0x5360, 0x074C, 0x5360, 0x074C, 0x5360, 0xFFFF, 0x02F1 };

   zmq::const_buffer buf1 = zmq::buffer(dh->GetData(), dh->GetSize());
   zmq::const_buffer buf2 = zmq::buffer(event, sizeof(event));

   args->data_sock->send(buf1, zmq::send_flags::sndmore);
   args->data_sock->send(buf2, zmq::send_flags::dontwait);

   // to send a log line:
   // args->m_data->services->SendLog("message abc",3);
   
   // wait for reply
   zmq::message_t reply;
   args->data_sock->recv(reply, zmq::recv_flags::none);

   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
