#include "RTC.h"
#include "io.h"

#define RTC_ADDRESS 0x70
#define RTC_DATA    0x71
#define RTC_BASE_YEAR 2000
#define TIME_ZONE_OFFSET 3

/**
 * @brief Reads a specific RTC register.
 * 
 * @param reg The register number to read from.
 * @return The value of the register.
 */
static uint8_t read_rtc_register(uint8_t reg);

/**
 * @brief Converts a Binary Coded Decimal (BCD) value to binary.
 * 
 * @param val The BCD value to convert.
 * @return The converted binary value.
 */
static uint8_t bcd_to_bin(uint8_t val);

static uint8_t read_rtc_register(uint8_t reg)
{
    out_byte(RTC_ADDRESS, reg);
    return in_byte(RTC_DATA);
}

static uint8_t bcd_to_bin(uint8_t val) 
{
    //example for BCD 45 = 0100(4) 0101(5)
    //to convert to bin we do (val & 0x0F) to return the first digit in the example 5
    //and add it to the shift of the 4 upper bits multiplying by 10 (45/16 = 4 * 10 = 40)
    //40 + 5 = 45
    return (val & 0x0F) + ((val / 16) * 10);
}

void get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) 
{
    *seconds = bcd_to_bin(read_rtc_register(0x00));
    *minutes = bcd_to_bin(read_rtc_register(0x02));
    *hours   = bcd_to_bin(read_rtc_register(0x04));

    *hours += TIME_ZONE_OFFSET;

    if(*hours>24)
    {
        *hours -= 24;
    }
}

void get_date(uint16_t *year, uint8_t *month, uint8_t *day) 
{
    *day   = bcd_to_bin(read_rtc_register(0x07));
    *month = bcd_to_bin(read_rtc_register(0x08));
    *year  = bcd_to_bin(read_rtc_register(0x09)) + RTC_BASE_YEAR;  //(RTC returns only the last two digits)
}
