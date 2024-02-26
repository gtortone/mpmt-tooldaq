#include <RunControl.h>

RunControl::RunControl(zmq::socket_t& insock, Logger& inlogger, Store& invariables) {

   sock = &insock;
   variables = &invariables;
   logger = &inlogger;

   // endpoint and port to bind run control socket to
   std::string run_control_sock_port = "tcp://*:11111";        
   // number of milliseconds before giving up on a connection if other side goes down
   int run_control_timeout;    

   variables->Get("UUID", UUID);
   variables->Get("run_control_sock_port", run_control_sock_port);
   variables->Get("run_control_timeout", run_control_timeout);

   sock->bind(run_control_sock_port.c_str());
   sock->setsockopt(ZMQ_RCVTIMEO, run_control_timeout);
   sock->setsockopt(ZMQ_SNDTIMEO, run_control_timeout);
}

bool RunControl::Send() {

   bool ret = true;

   if(msg_queue.size() > 0) {

      if(msg_queue.at(0).size() > 0) {

         for(int i = 0; i < msg_queue.at(0).size() - 1; i++) {
            std::cout << *(msg_queue.at(0).at(i)) << std::endl;
            ret *= sock->send(*(msg_queue.at(0).at(i)), ZMQ_SNDMORE);
         }

         std::cout << *(msg_queue.at(0).at(msg_queue.at(0).size() -1)) << std::endl;
         ret *= sock->send(*(msg_queue.at(0).at(msg_queue.at(0).size() - 1)));
      }

      msg_queue.pop_front();
   }

   if(!ret) {
      logger->Send("Warning!!! RunControl reply not sending correctly");
      return false;
   }

   return true;
}

bool RunControl::Receive() {

   zmq::message_t *identity = new zmq::message_t;

   std::cout << "in RunControl::Receive method" << std::endl;

   if(sock->recv(identity)) {

      std::cout << "identity: " << *identity << std::endl;

      zmq::message_t throwaway;
      if(identity->more() && sock->recv(&throwaway)) {
         
         std::cout << "throwaway: " << throwaway << std::endl;

         if(throwaway.more() && configuration_variables.Receive(sock)) {

            std::cout << "configuration_variables received" << std::endl;

            Store ret;

            ret.Set("msg_type", "Command Reply");

            std::string type = "";
            if(configuration_variables.Get("msg_type", type)) {

               std::cout << "msg_type: " << type << std::endl;

               if(type == "Config") {

                  ret.Set("AKN", Config());
                  logger->Send("MPMT Reconfigured/Initialised");
                  ret.Set("msg_value", "Received Config");
                  int state = 0;
                  variables->Get("state", state);

                  if(state == 1)
                     variables->Set("state", "2");

               } else if(type == "Command") {

                  std::string command = "";

                  if(configuration_variables.Get("msg_value", command)) {

                     ret.Set("msg_value", Command(command));
                     ret.Set("AKN", true);

                  } else {

                     ret.Set("msg_value", "Error!!! No msg_value/Command in JSON");
                     ret.Set("AKN", false);
                     logger->Send("Warning!!! No RunControl msg_value/Command in JSON");
                  }

               } else {

                  ret.Set("msg_value", "Error!!! Unknown msg_type");
                  ret.Set("AKN", false);
                  logger->Send("Warning!!! Unknown RunControl msg_type");
               }

               // reply

               long msg_id = 0;
               configuration_variables.Get("msg_id", msg_id);

               ret.Set("uuid", UUID);
               ret.Set("msg_id", msg_id);
               ret.Set("recv_time",
                       boost::posix_time::to_iso_extended_string
                       (boost::posix_time::second_clock::universal_time()));

               std::vector <zmq::message_t *> reply;
               reply.push_back(identity);

               zmq::message_t *blank = new zmq::message_t(0);
               reply.push_back(blank);

               reply.push_back(ret.MakeMsg());

               msg_queue.push_back(reply);

            } else logger->Send("Warning!!! RunControl received unknown message format");

            return false;

         } else logger->Send("Warning!!! RunControl received bad message");

      } else logger->Send("Warning!!! RunControl received bad message");

   } else logger->Send("Warning!!! RunControl received bad message");

   return true;
}

std::string RunControl::Command(std::string command) {

   // can add any run control commands you want to here

   std::string ret = "Warning!!! unknown Command received: " + command;

   int state = 0;
   variables->Get("state", state);

   if(command == "Start") {

      if(state == 2)
         state = 3;
      ret = "Started";
      logger->Send("MPMT Starting data taking");

   } else if(command == "Pause") {

      if(state == 3)
         state = 2;
      ret = "Paused";
      logger->Send("MPMT Paused");

   } else if(command == "Stop") {

      state = 0;
      ret = "Stopping";
      logger->Send("MPMT Shutting down");

   } else if(command == "Init") {

      state = 1;
      ret = "Initialising";

   } else if(command == "Status") {
      variables->Get("status", ret);

   } else if(command == "?") {

      if(state == 1)
         ret = "Available commands are: Stop, Status, ?";
      else
         ret = "Available commands are: Start, Pause, Stop, Init, Status, ?";
   }

   variables->Set("state", state);

   return ret;
}

bool RunControl::Config() {

   //  here you can make use of any of the configuration variables loaded in when loading the configuration JSON file.

   return true;
}

bool RunControl::Request() {

   std::vector <zmq::message_t *> reply;

   std::string ident = "MPMTRC1";

   zmq::message_t *identity = new zmq::message_t(8);

   snprintf((char *) identity->data(), ident.length() + 1, "%s", ident.c_str());

   reply.push_back(identity);

   zmq::message_t *blank = new zmq::message_t(0);

   reply.push_back(blank);

   Store tmp;
   tmp.Set("uuid", UUID);
   tmp.Set("msg_type", "Request");
   tmp.Set("msg_value", "MPMT");
   tmp.Set("time",
           boost::posix_time::to_iso_extended_string(boost::posix_time::
                                                     second_clock::universal_time()));

   reply.push_back(tmp.MakeMsg());

   msg_queue.push_back(reply);

   logger->Send("MPMT Initialising");

   return true;
}
