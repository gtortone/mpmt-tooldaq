#include "HVDevice.h"
#include "HVExceptions.h"

HVDevice::HVDevice(std::string port) {

   m_port = port;
   ctx = modbus_new_rtu(port.c_str(), 115200, 'N', 8, 1);
   
   if(ctx == NULL)
      throw(HVModbusError("unable to set modbus context"));

   if(modbus_connect(ctx) == -1) {
      throw(HVModbusError("unable to connect to modbus device"));
   }
}

HVDevice::~HVDevice() {

   modbus_close(ctx);
   modbus_free(ctx);
}

void HVDevice::Probe(void) {

   uint16_t test;

   for(int i=1; i<=20; i++) {
      modbus_set_slave(ctx, i);
      if(modbus_read_registers(ctx, 0, 1, &test) == -1)
         continue;
      channelList.push_back(i);
   }
}

std::vector<uint8_t> HVDevice::GetChannelList(void) {
   return channelList;
}

bool HVDevice::FindChannel(uint8_t id) {
   if(std::find(channelList.begin(), channelList.end(), id) == channelList.end())
      throw(HVChannelNotFound("HV channel not found"));

   return true;
}

// helper for slow control
void HVDevice::Set(uint8_t id, int value, std::string label) {

   FindChannel(id);
   
   if(std::find(param.begin(), param.end(), label) == param.end())
      throw(HVParamNotFound("HV parameter not found"));

   if(label == "power")
      SetPower(id, value);
   else if(label == "voltage_level")
      SetVoltageLevel(id, value);
   else if(label == "rampup_rate")
      SetRampupRate(id, value);
   else if(label == "rampdown_rate")
      SetRampdownRate(id, value);
   else if(label == "voltage_limit")
      SetVoltageLimit(id, value);
   else if(label == "current_limit")
      SetCurrentLimit(id, value);
   else if(label == "temperature_limit")
      SetTemperatureLimit(id, value);
   else if(label == "triptime_limit")
      SetTriptimeLimit(id, value);
   else if(label == "threshold")
      SetThreshold(id, value);
}

// Modbus methods

void HVDevice::Reset(uint8_t id) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 2, true);
   mbus.unlock();
}

void HVDevice::SetPower(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 1, value!=0);
   mbus.unlock();
}

void HVDevice::SetVoltageLevel(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x26, value);
   mbus.unlock();
}

void HVDevice::SetRampupRate(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x23, value);
   mbus.unlock();
}

void HVDevice::SetRampdownRate(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x24, value);
   mbus.unlock();
}

void HVDevice::SetVoltageLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x27, value);
   mbus.unlock();
}

void HVDevice::SetCurrentLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x25, value);
   mbus.unlock();
}

void HVDevice::SetTemperatureLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x2F, value);
   mbus.unlock();
}

void HVDevice::SetTriptimeLimit(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x22, value);
   mbus.unlock();
}

void HVDevice::SetThreshold(uint8_t id, int value) {

   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x2D, value);
   mbus.unlock();
}

int HVDevice::GetPowerStatus(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x06, 1, &value);
   mbus.unlock();
   return value;
}

double HVDevice::GetVoltageLevel(uint8_t id) {

   uint16_t msb, lsb;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2A, 1, &lsb);
      modbus_read_registers(ctx, 0x2B, 1, &msb);
   mbus.unlock();

   return ((msb << 16) + lsb) / 1000.0;
}

double HVDevice::GetCurrent(uint8_t id) {

   uint16_t msb, lsb;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x28, 1, &lsb);
      modbus_read_registers(ctx, 0x29, 1, &msb);
   mbus.unlock();

   return ((msb << 16) + lsb) / 1000.0;
}

int HVDevice::GetTemperature(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x07, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetRampupRate(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x23, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetRampdownRate(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x24, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetVoltageLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x27, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetCurrentLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x25, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetTemperatureLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2F, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetTriptimeLimit(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x22, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetThreshold(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2D, 1, &value);
   mbus.unlock();

   return value;
}

int HVDevice::GetAlarm(uint8_t id) {

   uint16_t value;
   mbus.lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2E, 1, &value);
   mbus.unlock();

   return value;
}
