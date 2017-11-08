#ifndef pr1_util_h
#define pr1_util_h


//These indicate the maximum number of characters
//required to store a number of each type.
#define FLOAT_MAX_LENGTH (3 + DBL_MANT_DIG - DBL_MIN_EXP)
#define SHORT_MAX_CHARS  3
#define ULONG_MAX_CHARS 10


#include <stdio.h>
#include <float.h>


size_t ultostr(unsigned long num, char *str);
size_t getTokenLength(const char *str, const size_t strLength, const char *delims);
void loadConfig(char *configPath, char **ip, size_t *ipLength, unsigned short *port, size_t *bufferSize);


#endif