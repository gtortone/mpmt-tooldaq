#include "HVDevice.h"
#include "HVExceptions.h"

HVDevice::HVDevice(std::string mode, std::string port, std::string hostip) {

   m_mode = mode;
   if(mode == "rtu") {
      m_port = port;
      ctx = modbus_new_rtu(port.c_str(), 115200, 'N', 8, 1);
   } else if(mode == "tcp") {
      ctx = modbus_new_tcp(hostip.c_str(), 502);
   }
   
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

void HVDevice::SetChannelList(std::vector<uint8_t> chlist) {
   channelList = chlist;
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

void HVDevice::mbus_lock(void) {
   if (m_mode == "tcp")
      return
   mbus.lock();
}

void HVDevice::mbus_unlock(void) {
   if (m_mode == "tcp")
      return
   mbus.unlock();
}

// Modbus methods

void HVDevice::Reset(uint8_t id) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 2, true);
   mbus_unlock();
}

void HVDevice::SetPower(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_bit(ctx, 1, value!=0);
   mbus_unlock();
}

void HVDevice::SetVoltageLevel(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x26, value);
   mbus_unlock();
}

void HVDevice::SetRampupRate(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x23, value);
   mbus_unlock();
}

void HVDevice::SetRampdownRate(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x24, value);
   mbus_unlock();
}

void HVDevice::SetVoltageLimit(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x27, value);
   mbus_unlock();
}

void HVDevice::SetCurrentLimit(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x25, value);
   mbus_unlock();
}

void HVDevice::SetTemperatureLimit(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x2F, value);
   mbus_unlock();
}

void HVDevice::SetTriptimeLimit(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x22, value);
   mbus_unlock();
}

void HVDevice::SetThreshold(uint8_t id, int value) {

   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_write_register(ctx, 0x2D, value);
   mbus_unlock();
}

int HVDevice::GetPowerStatus(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x06, 1, &value);
   mbus_unlock();
   return value;
}

double HVDevice::GetVoltageLevel(uint8_t id) {

   uint16_t msb, lsb;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2A, 1, &lsb);
      modbus_read_registers(ctx, 0x2B, 1, &msb);
   mbus_unlock();

   return ((msb << 16) + lsb) / 1000.0;
}

double HVDevice::GetCurrent(uint8_t id) {

   uint16_t msb, lsb;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x28, 1, &lsb);
      modbus_read_registers(ctx, 0x29, 1, &msb);
   mbus_unlock();

   return ((msb << 16) + lsb) / 1000.0;
}

float HVDevice::GetTemperature(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x07, 1, &value);
   mbus_unlock();

   float q = (value & 0xFF) / 1000.0;
   int i = (value >> 8) & 0xFF;   

   return q+i;
}

int HVDevice::GetRampupRate(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x23, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetRampdownRate(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x24, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetVoltageLimit(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x27, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetCurrentLimit(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x25, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetTemperatureLimit(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2F, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetTriptimeLimit(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x22, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetThreshold(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2D, 1, &value);
   mbus_unlock();

   return value;
}

int HVDevice::GetAlarm(uint8_t id) {

   uint16_t value;
   mbus_lock();
      modbus_set_slave(ctx, id);
      modbus_read_registers(ctx, 0x2E, 1, &value);
   mbus_unlock();

   return value;
}
