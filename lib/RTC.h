#ifndef RTC_H
#define RTC_H

#include <stdint.h>

// Function declarations for RTC operations

/**
 * @brief Reads a specific RTC register.
 * 
 * @param reg The register number to read from.
 * @return The value of the register.
 */
uint8_t read_rtc_register(uint8_t reg);

/**
 * @brief Converts a Binary Coded Decimal (BCD) value to binary.
 * 
 * @param val The BCD value to convert.
 * @return The converted binary value.
 */
uint8_t bcd_to_bin(uint8_t val);

/**
 * @brief Gets the current time from the RTC.
 * 
 * @param hours   Pointer to store the current hour.
 * @param minutes Pointer to store the current minute.
 * @param seconds Pointer to store the current second.
 */
void get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

/**
 * @brief Gets the current date from the RTC.
 * 
 * @param year  Pointer to store the current year.
 * @param month Pointer to store the current month.
 * @param day   Pointer to store the current day of the month.
 */
void get_date(uint16_t *year, uint8_t *month, uint8_t *day);

#endif
