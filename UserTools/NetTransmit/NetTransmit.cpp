#include "NetTransmit.h"

NetTransmit_args::NetTransmit_args():Thread_args(){ 
}

NetTransmit_args::~NetTransmit_args() {
}

NetTransmit::NetTransmit():Tool() {
}

bool NetTransmit::Initialise(std::string configfile, DataModel &data) {
  
   InitialiseTool(data);
   InitialiseConfiguration(configfile);

   if(!m_variables.Get("verbose",m_verbose)) 
      m_verbose = 1;

   m_util=new Utilities();

   m_data->data_sock = new zmq::socket_t(*(m_data->context), ZMQ_DEALER);
   // socket close shall block until either all pending messages have been sent, or the linger period (3s) expires
   m_data->data_sock->setsockopt(ZMQ_LINGER, 0);
   m_data->data_port = "4444";
   m_data->data_sock->set(zmq::sockopt::routing_id, std::to_string(m_data->mpmt_id));   
   m_data->data_sock->bind("tcp://*:" + m_data->data_port);

   args=new NetTransmit_args();
   args->m_data = m_data;
   args->buffer_id = &buffer_id;
   args->verbose = m_verbose;
   args->next_xfer = &next_xfer;
   
   m_util->CreateThread("transmit", &Thread, args);

   // prepare for first transfer
   next_xfer = true;
   
   ExportConfiguration();
   
   return true;
}

bool NetTransmit::Execute(){

   return true;
}

bool NetTransmit::Finalise() {

   if(m_verbose > 1)
      printf("NetTransmit::Finalise in progress\n");

   m_util->KillThread(args);
   printf("NetTransmit::Finalise KillThread done\n");

   m_data->data_sock->close();
   
   delete args;
   args=0;
   
   delete m_util;
   m_util=0;
   
   if(m_verbose > 1)
      printf("NetTransmit::Finalise in done\n");

   return true;
}

void NetTransmit::Thread(Thread_args* arg) {

   NetTransmit_args* args=reinterpret_cast<NetTransmit_args*>(arg);
   zmq::message_t reply;
 
   DAQHeader *dh = new DAQHeader();
   uint16_t *bufusint;
 
   if(*(args->next_xfer)) {
 
      if(args->verbose > 1)
         printf("NetTransmit::Thread fetch new buffer_id\n");
 
      bool timeout;
      *(args->buffer_id) = args->m_data->q2.pop(&timeout);

      if(timeout) {
         if(args->verbose > 1)
            printf("NetTransmit::Thread transmit queue empty\n");
         return;
      }
 
      if(args->verbose > 1)
         printf("NetTransmit::Thread buffer_id: %d\n", *(args->buffer_id));
   }
 
   dh->SetCardType(2);
   dh->SetCardID(args->m_data->mpmt_id); 
   // FIXME 
   dh->SetCoarseCounter(800);
 
   bufusint = reinterpret_cast<uint16_t *>(args->m_data->buf_ptr[*(args->buffer_id)].buffer);
 
   zmq::const_buffer buf1 = zmq::buffer(dh->GetData(), dh->GetSize());
   zmq::const_buffer buf2 = zmq::buffer(bufusint, args->m_data->buf_ptr[*(args->buffer_id)].length);
 
   if(args->verbose > 1)
       printf("NetTransmit::Thread send in progress (%d)\n", *(args->buffer_id));
 
   args->m_data->data_sock->send(buf1, zmq::send_flags::sndmore | zmq::send_flags::dontwait);
   auto rc = args->m_data->data_sock->send(buf2, zmq::send_flags::dontwait);
 
   if(rc.has_value()) {
 
      // if transfer is ok rc.value() is equal to buffer size
 
      if(rc.value() != args->m_data->buf_ptr[*(args->buffer_id)].length) {
 
         if(args->verbose > 1)
            printf("NetTransmit::Thread partial buffer sent (%d)\n", *(args->buffer_id));
      
      } else {    // send success
 
         args->m_data->q1.push(*(args->buffer_id));
 
         if(args->verbose > 1) {
            printf("NetTransmit::Thread send done (%d)\n", *(args->buffer_id));
            printf("NetTransmit::Thread recv ack in progress (%d)\n", *(args->buffer_id));
         }
 
         auto rc = args->m_data->data_sock->recv(reply, zmq::recv_flags::dontwait);
 
         if(args->verbose > 1) {
            if(rc.has_value())
               printf("NetTransmit::Thread recv ack done (%d)\n", *(args->buffer_id));
            else
               printf("NetTransmit::Thread recv ack failed (%d)\n", *(args->buffer_id));
         }
 
         *(args->next_xfer) = true;
      }
 
   } else {
 
      if(errno == EAGAIN) {
         if(args->verbose > 1) 
            printf("NetTransmit::Thread send failed (%d)\n", *(args->buffer_id));
 
         *(args->next_xfer) = false;
         std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
   }
}
