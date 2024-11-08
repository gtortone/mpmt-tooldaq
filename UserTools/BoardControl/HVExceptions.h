#ifndef HVExceptions_H
#define HVExceptions_H

class HVModbusError : public std::exception {

private:
   std::string message;

public:
   HVModbusError(const char* msg) : message(msg) {}

   const char* what() const throw() {
       return message.c_str();
   }
};

class HVChannelNotFound : public std::exception {

private:
   std::string message;

public:
   HVChannelNotFound(const char* msg) : message(msg) {}

   const char* what() const throw() {
       return message.c_str();
   }
};

class HVParamNotFound : public std::exception {

private:
   std::string message;

public:
   HVParamNotFound(const char* msg) : message(msg) {}

   const char* what() const throw() {
       return message.c_str();
   }
};

class HVParamParseError : public std::exception {

private:
   std::string message;

public:
   HVParamParseError(const char* msg) : message(msg) {}

   const char* what() const throw() {
       return message.c_str();
   }
};

class HVParamValueNotValid : public std::exception {

private:
   std::string message;

public:
   HVParamValueNotValid(const char* msg) : message(msg) {}

   const char* what() const throw() {
       return message.c_str();
   }
};

#endif
