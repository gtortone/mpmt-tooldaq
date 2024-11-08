#ifndef RCExceptions_H
#define RCExceptions_H

class RCDeviceError : public std::exception {

private:
   std::string message;

public:
   RCDeviceError(const char* msg) : message(msg) {}

   const char* what() const throw() {
       return message.c_str();
   }
};

#endif
