#ifndef _my_time_h__
#define _my_time_h__

#include <mysql/my_global.h>
#include <mysql/mysql_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flags to str_to_datetime and number_to_datetime */
#define TIME_FUZZY_DATE         1
#define TIME_DATETIME_ONLY      2
#define TIME_NO_NSEC_ROUNDING   4
#define TIME_NO_DATE_FRAC_WARN  8

/*
  Required buffer length for my_time_to_str, my_date_to_str,
  my_datetime_to_str and TIME_to_string functions. Note, that the
  caller is still responsible to check that given TIME structure
  has values in valid ranges, otherwise size of the buffer could
  be not enough. We also rely on the fact that even wrong values
  sent using binary protocol fit in this buffer.
*/
#define MAX_DATE_STRING_REP_LENGTH 30

longlong my_datetime_packed_from_binary(const uchar *ptr, uint dec);
uint my_datetime_binary_length(uint dec);

longlong my_time_packed_from_binary(const uchar *ptr, uint dec);
uint my_time_binary_length(uint dec);

void my_timestamp_from_binary(struct timeval *tm, const uchar *ptr, uint dec);
uint my_timestamp_binary_length(uint dec);

longlong number_to_datetime(longlong nr, MYSQL_TIME *time_res,
                            ulonglong flags, int *was_cut);

my_bool number_to_time(longlong nr, MYSQL_TIME *ltime, int *warnings);

int my_TIME_to_str(const MYSQL_TIME *l_time, char *to, uint dec);

void TIME_from_longlong_datetime_packed(MYSQL_TIME *ltime, longlong nr);
void TIME_from_longlong_time_packed(MYSQL_TIME *ltime, longlong nr);
void TIME_from_longlong_date_packed(MYSQL_TIME *ltime, longlong nr);

#ifdef __cplusplus
}
#endif
#endif /* _my_time_h__ */
// vim: et ts=4
