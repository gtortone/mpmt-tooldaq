#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define I2CDeviceDefaultName "/dev/i2c-0"

#define I2CADDRESS_1                 (0x48)    // 1001 000 (ADDR = GND)
#define I2CADDRESS_2                 (0x49)    // 1001 000 (ADDR = VDD)
#define I2CADDRESS_3                 (0x4B)    // 1001 000 (ADDR = SCL)

#define TLA2024_CONVERSIONDELAY (1000) ///< Conversion delay

#define TLA2024_REG_POINTER_CONVERT (0x00)   ///< Conversion
#define TLA2024_REG_POINTER_CONFIG (0x01)    ///< Configuration

#define TLA2024_REG_CONFIG_OS_MASK (0x8000) ///< OS Mask
#define TLA2024_REG_CONFIG_OS_SINGLE  (0x8000) ///< Write: Set to start a single-conversion
#define TLA2024_REG_CONFIG_OS_BUSY (0x0000) ///< Read: Bit = 0 when conversion is in progress
#define TLA2024_REG_CONFIG_OS_NOTBUSY  (0x8000) ///< Read: Bit = 1 when device is not performing a conversion

#define TLA2024_REG_CONFIG_MUX_DIFF_0_1 (0x0000) ///< Differential P = AIN0, N = AIN1 (default)
#define TLA2024_REG_CONFIG_MUX_DIFF_0_3 (0x1000) ///< Differential P = AIN0, N = AIN3
#define TLA2024_REG_CONFIG_MUX_DIFF_1_3 (0x2000) ///< Differential P = AIN1, N = AIN3
#define TLA2024_REG_CONFIG_MUX_DIFF_2_3 (0x3000) ///< Differential P = AIN2, N = AIN3
#define TLA2024_REG_CONFIG_MUX_SINGLE_0 (0x4000) ///< Single-ended AIN0
#define TLA2024_REG_CONFIG_MUX_SINGLE_1 (0x5000) ///< Single-ended AIN1
#define TLA2024_REG_CONFIG_MUX_SINGLE_2 (0x6000) ///< Single-ended AIN2
#define TLA2024_REG_CONFIG_MUX_SINGLE_3 (0x7000) ///< Single-ended AIN3

#define TLA2024_REG_CONFIG_PGA_6_144V (0x0000) ///< +/-6.144V
#define TLA2024_REG_CONFIG_PGA_4_096V (0x0200) ///< +/-4.096V
#define TLA2024_REG_CONFIG_PGA_2_048V (0x0400) ///< +/-2.048V
#define TLA2024_REG_CONFIG_PGA_1_024V (0x0600) ///< +/-1.024V
#define TLA2024_REG_CONFIG_PGA_0_512V (0x0800) ///< +/-0.512V
#define TLA2024_REG_CONFIG_PGA_0_256V (0x0A00) ///< +/-0.256V

#define TLA2024_REG_CONFIG_MODE_CONTIN (0x0000) ///< Continuous conversion mode
#define TLA2024_REG_CONFIG_MODE_SINGLE (0x0100) ///< Power-down single-shot mode (default)

#define TLA2024_REG_CONFIG_DR_128SPS (0x0000) ///< 128 samples per second
#define TLA2024_REG_CONFIG_DR_250SPS (0x0020) ///< 250 samples per second
#define TLA2024_REG_CONFIG_DR_490SPS (0x0040) ///< 490 samples per second
#define TLA2024_REG_CONFIG_DR_920SPS (0x0060) ///< 920 samples per second
#define TLA2024_REG_CONFIG_DR_1600SPS (0x0080) ///< 1600 samples per second (default)
#define TLA2024_REG_CONFIG_DR_2400SPS (0x00A0) ///< 2400 samples per second
#define TLA2024_REG_CONFIG_DR_3300SPS (0x00C0) ///< 3300 samples per second
#define ADS1115_REG_CONFIG_DR_860SPS  (0x00E0)  // 860 samples per second for ADS1115 (3300 for TLA2024)

#define TLA2024_REG_RESERVED (0x0003)  ///< Reserved section must be written 03h

/** Gain settings */
typedef enum {
    GAIN_TWOTHIRDS = TLA2024_REG_CONFIG_PGA_6_144V,
    GAIN_ONE = TLA2024_REG_CONFIG_PGA_4_096V,
    GAIN_TWO = TLA2024_REG_CONFIG_PGA_2_048V,
    GAIN_FOUR = TLA2024_REG_CONFIG_PGA_1_024V,
    GAIN_EIGHT = TLA2024_REG_CONFIG_PGA_0_512V,
    GAIN_SIXTEEN = TLA2024_REG_CONFIG_PGA_0_256V
} tlaGain_t;

/** Sampling settings */
typedef enum
{
    SPS_128 = TLA2024_REG_CONFIG_DR_128SPS,
    SPS_250 = TLA2024_REG_CONFIG_DR_250SPS,
    SPS_490 = TLA2024_REG_CONFIG_DR_490SPS,
    SPS_920 = TLA2024_REG_CONFIG_DR_920SPS,
    SPS_1600 = TLA2024_REG_CONFIG_DR_1600SPS,
    SPS_2400 = TLA2024_REG_CONFIG_DR_2400SPS,
    SPS_3300 = TLA2024_REG_CONFIG_DR_3300SPS,
    SPS_860 = ADS1115_REG_CONFIG_DR_860SPS
} tlaSps_t;

class TLA2024 {
protected:
    // Instance-specific properties
    int m_i2cHandle;
    const char* m_i2cDeviceName;
    uint8_t m_i2cAddress;      ///< the I2C address
    uint16_t m_conversionDelay; ///< conversion deay
    uint8_t m_bitShift;        ///< bit shift amount
    tlaGain_t m_gain;          ///< ADC gain
    tlaSps_t  m_sps;
    uint8_t   m_tlaType;

public:
    TLA2024(const char* i2cDeviceName = I2CDeviceDefaultName, uint8_t i2cAddress = I2CADDRESS_1);
    uint16_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint16_t value);
    uint16_t readADC_SingleEnded(uint8_t channel);
    int16_t readADC_Differential_0_1(void);
    int16_t readADC_Differential_2_3(void);
    int16_t getLastConversionResults();
    void updateI2cDevice(const char* i2cDeviceName);
    void setGain(tlaGain_t gain);
    tlaGain_t getGain(void);
    void      setSps(tlaSps_t sps);
    tlaSps_t  getSps(void);
    void      setConversionDelay(void);

private:
};
