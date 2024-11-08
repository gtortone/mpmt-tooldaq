#ifndef RC_DEVICE_H 
#define RC_DEVICE_H

#include <string>
#include <cstdint>

#define MAP_SIZE     0x10000

class RCDevice {

private:
   std::string m_port;
   int fd;
   volatile uint32_t *ptr;

public:

   RCDevice(std::string port);
   ~RCDevice();

   void WriteRegister(uint16_t addr, uint32_t value);
   uint32_t ReadRegister(uint16_t addr);
};

#endif
