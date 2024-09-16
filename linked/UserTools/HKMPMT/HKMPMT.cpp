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
   uint16_t event[8] = { 0xbaab, 0xc064, 0x9177, 0x3622, 0x210b, 0x80e0, 0x001c, 0xfeef };

   zmq::const_buffer buf1 = zmq::buffer(dh->GetData(), dh->GetSize());
   zmq::const_buffer buf2 = zmq::buffer(event, 8);

   args->data_sock->send(buf1, zmq::send_flags::sndmore);
   args->data_sock->send(buf2, zmq::send_flags::dontwait);

   // to send a log line:
   // args->m_data->services->SendLog("message abc",3);
   
   // wait for reply
   zmq::message_t reply;
   args->data_sock->recv(reply, zmq::recv_flags::none);

   //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
