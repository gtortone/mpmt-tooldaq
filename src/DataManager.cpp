#include <DataManager.h>

DataManager::DataManager(zmq::socket_t& insock, Logger& inlogger, Store& invariables) {

   sock = &insock;
   logger = &inlogger;
   variables = &invariables;

   // endpoint and port to bind data socket
   std::string data_sock_port = "tcp://*:44444";        
   // number of milliseconds before giving up on a connection if other side goes down
   int data_timeout = 100;      

   // number of milliseconds before resend of data if no acknoledgement from DAQ has been received
   int resend_time = 10;        

   variables->Get("data_sock_port", data_sock_port);
   variables->Get("data_timeout", data_timeout);
   variables->Get("UUID", UUID);
   variables->Get("data_timeout", data_timeout);

   variables->Get("resend_period", resend_time);
   resend_period = boost::posix_time::milliseconds(resend_time);

   variables->Get("resend_attempts", resend_attempts);

   // number of data chunks in memory before warning that nearfull capacity
   if(!variables->Get("queue_warning_limit", queue_warning_limit))
      queue_warning_limit = 8000;       

   // maximum capacity of data chunks, when reached oldest data will be deleted to make room for new data 
   if(!variables->Get("queue_max_size", queue_max_size))
      queue_max_size = 10000;   

   // number of ms to collect hits into a single data chunk
   if(!variables->Get("data_chunk_size_ms", data_chunk_size_ms))
      data_chunk_size_ms = 100; 

   sock->bind(data_sock_port.c_str());
   sock->setsockopt(ZMQ_RCVTIMEO, data_timeout);
   sock->setsockopt(ZMQ_SNDTIMEO, data_timeout);

   // rate in Hz of PMTs for creating fake MPMT data 5700= 19(PMTs) x 300(Hz)
   if(!variables->Get("fake_data_rate", fake_data_rate))
      fake_data_rate = 5700;    

   last_get = boost::posix_time::microsec_clock::universal_time();

   data_id = 0;
}

bool DataManager::GetData() {

   boost::posix_time::time_duration lapse = boost::posix_time::microsec_clock::universal_time() - last_get;

   long ms = lapse.total_milliseconds();

   if(ms >= data_chunk_size_ms) {

      int numhits = (fake_data_rate * ms) / 1000;

      if(numhits > 0) {

         MPMTDataChunk *data = new MPMTDataChunk(UUID);
         data->hits.resize(numhits);

         for(int i = 0; i < numhits; i++) {

            data->hits.at(i).adc_charge = 1000 + i;
            data->hits.at(i).pmt_id = (i * 100) % 19;
            data->hits.at(i).time_corse = i;
            data->hits.at(i).time_fine = i * 100;
         }

         data_id++;
         data->data_id = data_id;
         data_queue.push_back(data);

         last_get = boost::posix_time::microsec_clock::universal_time();
      }
   }

   return true;

}

bool DataManager::ManageQueues() {

   if((data_queue.size() + sent_queue.size()) > queue_max_size) {

      logger->Send("Warnning!!! MPMT buffer full oldest data is being deleted");

      if(data_queue.size()) {

         delete data_queue.at(0);
         data_queue.at(0) = 0;
         data_queue.pop_front();
      }
   }
   else if((data_queue.size() + sent_queue.size()) > queue_warning_limit)
      logger->Send("Warnning!!! MPMT buffer nearly full");

   for(std::deque < MPMTDataChunk * >::iterator it = sent_queue.begin(); it != sent_queue.end(); it++) {

      boost::posix_time::time_duration lapse =
         resend_period - (boost::posix_time::microsec_clock::universal_time() - (*it)->last_send);

      if(lapse.is_negative()) {
         
         if((*it)->attempts < resend_attempts) {

            (*it)->attempts++;
            data_queue.push_front((*it));
         }
         else delete(*it);

         (*it) = 0;
         sent_queue.erase(it);
         break;
      }
   }

   return true;
}

bool DataManager::Send() {

   bool ret = true;

   if(data_queue.size()) {

      data_queue.at(0)->last_send = boost::posix_time::microsec_clock::universal_time();
      ret *= data_queue.at(0)->Send(sock);
      sent_queue.push_back(data_queue.at(0));
      data_queue.at(0) = 0;
      data_queue.pop_front();
   }

   if(!ret)
      logger->Send("WARNING!!! MPMT error sending data");

   return ret;
}

bool DataManager::Receive() {

   bool found = false;

   Store akn;

   if(akn.Receive(sock)) {

      int received_id = 0;
      if(akn.Get("data_id", received_id)) {

         for(std::deque < MPMTDataChunk * >::iterator it = sent_queue.begin(); it != sent_queue.end(); it++) {

            if((*it)->data_id == received_id) {
               delete(*it);
               (*it) = 0;
               sent_queue.erase(it);
               found = true;
               break;
            }
         }

         if(!found) {

            for(std::deque < MPMTDataChunk * >::iterator it = data_queue.begin(); it != data_queue.end(); it++) {

               if((*it)->data_id == received_id) {

                  delete(*it);
                  (*it) = 0;
                  data_queue.erase(it);
                  break;
               }
            }
         }

         return true;
      }

      logger->Send("Warning!!! MPMT data_id missing from data AKN");
      return false;
   }

   logger->Send("Warning!!! MPMT failed to receive data AKN");
   return false;
}

void DataManager::UpdateStatus() {

   variables->Set("data_queue", data_queue.size());
   variables->Set("sent_queue", sent_queue.size());
   variables->Set("max_queue", queue_max_size);
}
