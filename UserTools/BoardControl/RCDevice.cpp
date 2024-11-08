
#include "RCDevice.h"
#include "RCExceptions.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

RCDevice::RCDevice(std::string port) {

   m_port = port;
   fd = open(port.c_str(), O_RDWR);

   if(fd < 1)
      throw(RCDeviceError("failed to open UIO device"));

   ptr = (volatile uint32_t *) mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

RCDevice::~RCDevice() {

   munmap((uint32_t *) ptr, MAP_SIZE);
   close(fd);
}

uint32_t RCDevice::ReadRegister(uint16_t addr) {
   return ptr[addr];
}

void RCDevice::WriteRegister(uint16_t addr, uint32_t value) {
   ptr[addr] = value; 
}
