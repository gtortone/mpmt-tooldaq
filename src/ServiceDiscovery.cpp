#include <ServiceDiscovery.h>

ServiceDiscovery::ServiceDiscovery(Store& invariables, int &return_sock) {

   variables = &invariables;

   // service discovery multicast address (needs to be consistent with DAQ)
   std::string address = "239.192.1.1"; 
   // service discovery multicast port number (needs to be consistent with DAQ)
   int port = 5000;             
   std::string UUID;
   //service discovery name so other nodes know what type of node you are
   std::string service_name = "MPMT";   
   // port number for remote control/slow control
   int remote_port = 22222;     

   variables->Get("service_discovery_address", address);
   variables->Get("service_discovery_port", port);
   variables->Get("UUID", UUID);
   variables->Get("service_name", service_name);
   variables->Get("slow_control_port", remote_port);

   // setup socket

   sock = socket(AF_INET, SOCK_DGRAM, 0);
   struct linger l;
   l.l_onoff = 0;
   l.l_linger = 0;
   setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &l, sizeof(l));

   if(sock < 0) {
      perror("socket");
      printf("Failed to connect to multicast publish socket");
      exit(1);
   }
   bzero((char *) &addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
   addrlen = sizeof(addr);

   // send

   addr.sin_addr.s_addr = inet_addr(address.c_str());

   int m_remoteport = 00000;

   message.Set("uuid", UUID);
   message.Set("remote_port", remote_port);
   message.Set("status_query", true);
   message.Set("msg_type", "Service Discovery");
   message.Set("msg_value", service_name);

   msg_id = 0;

   return_sock = sock;
}

void ServiceDiscovery::Send() {

   msg_id++;

   std::string status;
   variables->Get("status", status);

   message.Set("status", status);
   message.Set("msg_id", msg_id);

   boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();

   std::stringstream isot;
   isot << boost::posix_time::to_iso_extended_string(t) << "Z";

   message.Set("msg_time", isot.str());

   std::string pubmessage;
   message >> pubmessage;

   char msg[2048];
   snprintf(msg, pubmessage.length() + 1, "%s", pubmessage.c_str());

   cnt = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &addr, addrlen);
}
