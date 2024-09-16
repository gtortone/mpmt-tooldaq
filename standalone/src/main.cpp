#include <iostream>
#include <string>
#include <sstream>

#include <zmq.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <Logger.h>
#include <Store.h>
#include <ServiceDiscovery.h>
#include <RunControl.h>
#include <SlowControl.h>
#include <Monitor.h>
#include <DataManager.h>

int main(int argc, char *argv[]) {

   if(argc > 2) {
      std::cout << "Incorrect number of arguments" << std::endl;
      std::cout << "usage: ./main <variables> " << std::endl;
      exit(1);
   }

   // Variables and Defaults

   // simple dynamic omni storage class
   Store variables;             

   // unique identifier
   std::string UUID = boost::uuids::to_string(boost::uuids::random_generator()());      

   // seconds between service discovery broadcasts
   int broadcast_period = 5;    
   // number of seconds between requests for initial configurat
   int config_request_period = 1;       
   // number of seconds between monitoring information is given out
   int monitor_send_period = 10;        
   // endpoint for service discovery socket
   int service_discovery_sock = 0;      

   variables.Set("UUID", UUID);
   variables.Set("broadcast_period", broadcast_period);
   variables.Set("config_request_period", config_request_period);
   variables.Set("monitor_send_period", monitor_send_period);

   if(argc == 2) {
      // loading dynamic variables file
      variables.Initialise(argv[1]);    

      // loading variables dynamically from variables file
      variables.Get("UUID", UUID);      
      variables.Get("broadcast_period", broadcast_period);
      variables.Get("config_request_period", config_request_period);
      variables.Get("monitor_send_period", monitor_send_period);

   }

   // Setting up sockets and classes 

   // ZMQ context for managing socket queues
   zmq::context_t context(1);   

   // socket for sending out logging/warning messages
   zmq::socket_t logger_sock(context, ZMQ_PUB); 
   // class for handling formatting and sending of logging messages
   Logger logger(logger_sock, variables);       

   // class for handling service discovery broadcasts
   ServiceDiscovery service_discovery(variables, service_discovery_sock);       
   
   // socket for run control input and output
   zmq::socket_t run_control_sock(context, ZMQ_ROUTER);        
   // class for handling slow control input, commands and output
   RunControl run_control(run_control_sock, logger, variables);      

   // socket for slow control input and output
   zmq::socket_t slow_control_sock(context, ZMQ_ROUTER);        
   // class for handling slow control input, commands and output
   SlowControl slow_control(slow_control_sock, logger, variables);      

   // socket for sending monitoring information out
   zmq::socket_t monitor_sock(context, ZMQ_PUB);        
   // class to handle collecting and sending monitoring data
   Monitor monitor(monitor_sock, variables);    

   // socket for sending out data and receiving acknolwedge statements
   zmq::socket_t data_sock(context, ZMQ_DEALER);        
   // class for handling data queues and collecting data
   DataManager data_manager(data_sock, logger, variables);      

   // input sockets to poll (similar to select) for activity 
   zmq::pollitem_t in_items[] = {       
      {run_control_sock, 0, ZMQ_POLLIN, 0},
      {slow_control_sock, 0, ZMQ_POLLIN, 0},
      {data_sock, 0, ZMQ_POLLIN, 0},
   };

   // output sockets to poll (similar to select) for activity
   zmq::pollitem_t out_items[] = {      
      {run_control_sock, 0, ZMQ_POLLOUT, 0},
      {slow_control_sock, 0, ZMQ_POLLOUT, 0},
      {NULL, service_discovery_sock, ZMQ_POLLOUT, 0},
      {monitor_sock, 0, ZMQ_POLLOUT, 0},
      {data_sock, 0, ZMQ_POLLOUT, 0},
   };

   // Timers 

   // converting service discovery period variable to boost time_duration
   boost::posix_time::time_duration broadcast_period_td = boost::posix_time::seconds(broadcast_period); 
   // variable for storing time of last service discovery broadcast
   boost::posix_time::ptime last_service_broadcast = boost::posix_time::second_clock::universal_time(); 

   // converting slow control configuration request period variable to boost time_duration    
   boost::posix_time::time_duration config_request_period_td = boost::posix_time::seconds(config_request_period);       
   // variable for storing time of last configuration request
   boost::posix_time::ptime last_config_request = boost::posix_time::second_clock::universal_time();    

   // converting monitoring period variable to boost time_duration
   boost::posix_time::time_duration monitor_send_period_td = boost::posix_time::seconds(monitor_send_period);   
   // variable for storing time of last monitoring braodcast
   boost::posix_time::ptime last_monitor_send = boost::posix_time::second_clock::universal_time();      

   //

   // variables to define the state of the system 0=Shutdown, 1=Initialisation, 2=Paused not taking data but initialised, 3=Taking data
   int state = 1;               

   // loading state variable into varaibles Store so all classes can access it
   variables.Set("state", state);       
   // loading current system status into variables Store so all classes can access it 
   variables.Set("status", "Booting");  

   // main execution loop while state is anything other than shutdown
   while(state) {               

      // if in data taking state
      if(state == 3) {          
         // Get data from hardware 
         data_manager.GetData();        
         // there is no blocking on the polling here to not limit/holdup the collection of data 
         zmq::poll(&in_items[0], 3, 0); 
      }
      else {
         // in any other state other than data taking i have a 100ms block on the poll to keep the CPU from maxing out doing nothing and power consumption low.
         zmq::poll(&in_items[0], 3, 100);       
      }

      // this function manages the two queues of data the new data queue that needs to be sent out and the sent queue that is data waiting to be deleted 
      // when an acknolwedgement is received. It also handles sending out warnings and deleteing old data if the queues get too large and moving data 
      // between the queues for resending.
      data_manager.ManageQueues();      
      // polling the outbound sockets. This poll has no block as can be delayed by the inbound socket polling. 
      zmq::poll(&out_items[0], 5, 0);   

      if(in_items[1].revents & ZMQ_POLLIN) {
         // reading in any data received on the slowcontrol port and taking the necessary actions
         std::cout << "in slow_control ZMQ_POLLIN" << std::endl;
         slow_control.Receive();
      }

      // determining if the the slow control configuration request timer has lapsed
      boost::posix_time::time_duration lapse = config_request_period_td - (boost::posix_time::second_clock::universal_time() - last_config_request);    

      // only want to send out these requests in the intialising state 
      if(lapse.is_negative() && state == 1) {   

         // sending out the request for configuration
         slow_control.Request();
         // resetting last time request sent;
         last_config_request = boost::posix_time::second_clock::universal_time();
      }

      if((out_items[1].revents & ZMQ_POLLOUT) && state != 0)
         // sending any data necesary on the slow control socket
         slow_control.Send();   

      // checking if service discovery time has lapsed
      lapse = broadcast_period_td - (boost::posix_time::second_clock::universal_time() - last_service_broadcast);       

      if((out_items[2].revents & ZMQ_POLLOUT) && lapse.is_negative() && state != 0) {

         // sending out service discovery braodcast
         service_discovery.Send();      
         // resetting last time
         last_service_broadcast = boost::posix_time::second_clock::universal_time();    
      }

      // checking if monitoring time has lapsed
      lapse = monitor_send_period_td - (boost::posix_time::second_clock::universal_time() - last_monitor_send); 

      if((out_items[3].revents & ZMQ_POLLOUT) && lapse.is_negative() && state != 0) {

         // updating the queue status into the variables Store
         data_manager.UpdateStatus();   
         // sending out monitoring info
         monitor.Send();        
         // resetting time of last send
         last_monitor_send = boost::posix_time::second_clock::universal_time(); 
      }

      if((in_items[2].revents & ZMQ_POLLIN) && state != 0) {
         // receiving any data acknolwedge statements
         data_manager.Receive();        
      }

      if((out_items[4].revents & ZMQ_POLLOUT) && state != 0) {
         // sending out any data
         data_manager.Send();
      }

      // get the current state from store in case classes have changed it           
      variables.Get("state", state);    
   }

   return 0;
}
