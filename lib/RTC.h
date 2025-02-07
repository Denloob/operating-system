#ifndef RTC_H
#define RTC_H

#include <stdint.h>

/**
 * @brief Gets the current time from the RTC.
 *
 * @param hours   Pointer to store the current hour.
 * @param minutes Pointer to store the current minute.
 * @param seconds Pointer to store the current second.
 */
void RTC_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

/**
 * @brief Gets the current date from the RTC.
 *
 * @param year  Pointer to store the current year.
 * @param month Pointer to store the current month.
 * @param day   Pointer to store the current day of the month.
 */
void RTC_get_date(uint16_t *year, uint8_t *month, uint8_t *day);

#endif
