/*
 * util.h
 *
 *  Created on: 13.04.2018
 *      Author: dk
 */

#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#define CLAMP(x, low, high) ({\
  __typeof__(x) __x = (x); \
  __typeof__(low) __low = (low);\
  __typeof__(high) __high = (high);\
  __x > __high ? __high : (__x < __low ? __low : __x);\
  })

char* get_config_by_key(char *key);

#endif /* SRC_UTIL_H_ */
