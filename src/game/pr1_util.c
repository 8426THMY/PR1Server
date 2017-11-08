#include "pr1_util.h"


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>


//Forward-declare our helper functions!
static char *readLineFile(FILE *file, char *line, size_t *lineLength);


//Convert an integer to a string.
//We assume that "str" points to an array with at least (ULONG_MAX_CHARS + 1) characters.
size_t ultostr(unsigned long num, char *str){
	//Special case for when num is equal to 0!
	if(num == 0){
		*str++ = '0';
		*str = '\0';

		return(1);
	}


	size_t length = 0;
	char *curPos = str;

	//Check for a minus sign.
	if(num < 0){
		*curPos = '-';
		num = -num;
	}else{
		*curPos = '\0';
	}

	//Add the digits backwards, starting at the end of the array.
	curPos += 10;
	while(num > 0){
		*curPos-- = '0' + num % 10;
		num /= 10;
		++length;
	}

	//Now copy them over to the front!
	if(*str != '-'){
		memcpy(str, curPos + 1, length * sizeof(*str));
	}else{
		if(length < 10){
			memcpy(str + 1, curPos + 1, length * sizeof(*str));
		}
		++length;
	}

	//Add a null terminator and we're set!
	str[length] = '\0';


	return(length);
}

//Get the length of a token!
size_t getTokenLength(const char *str, const size_t strLength, const char *delims){
	const char *tokStart = str;

	//Keep looping until we get to the end of the string or a null terminator!
	while(*tokStart != '\0' && tokStart - str < strLength){
		const char *curDelim = delims;
		while(*curDelim != '\0'){
			if(*tokStart == *curDelim){
				return(tokStart - str);
			}
			++curDelim;
		}
		++tokStart;
	}


	return(strLength);
}

void loadConfig(char *configPath, char **ip, size_t *ipLength, unsigned short *port, size_t *bufferSize){
	FILE *configFile = fopen(configPath, "rb");
	if(configFile != NULL){
		char lineBuffer[1024];
		char *line;
		size_t lineLength;

		unsigned int tempNum;
		char *endPos;


		while((line = readLineFile(configFile, lineBuffer, &lineLength)) != NULL){
			//Set the I.P.
			if(strncmp(line, "ip = ", 5) == 0){
				*ipLength = lineLength - 5;
				*ip = realloc(*ip, (*ipLength + 1) * sizeof(**ip));
				if(*ip != NULL){
					memcpy(*ip, line + 5, *ipLength * sizeof(**ip));
					(*ip)[*ipLength] = '\0';
				}

			//Set the port.
			}else if(strncmp(line, "port = ", 7) == 0){
				tempNum = strtol(line + 7, &endPos, 10);
				//Make sure it's valid!
				if(*endPos == '\0' && errno != ERANGE && tempNum < (USHRT_MAX * 2)){
					*port = tempNum;
				}

			//Set the buffer size.
			}else if(strncmp(line, "buff = ", 7) == 0){
				tempNum = strtol(line + 7, &endPos, 10);
				//Make sure it's valid!
				if(*endPos == '\0' && errno != ERANGE && tempNum > 0){
					*bufferSize = tempNum;
				}
			}
		}
	}else{
		printf("Unable to open config file!\n"
		       "Path: %s\n\n", configPath);
	}
	fclose(configFile);
}


static char *readLineFile(FILE *file, char *line, size_t *lineLength){
	line = fgets(line, 1024, file);
	if(line != NULL){
		*lineLength = strlen(line);

		//Remove comments.
		char *commentPos = strstr(line, "//");
		if(commentPos != NULL){
			*lineLength -= commentPos - line;
			*commentPos = '\0';
		}

		//"Remove" whitespace characters from the end of the line!
		size_t i;
		for(i = 0; i < *lineLength; ++i){
			if(!isspace(line[i])){
				line += i;
				*lineLength -= i;

				break;
			}
		}

		//"Remove" whitespace characters from the beginning of the line!
		for(i = *lineLength; i > 0; --i){
			if(!isspace(line[i - 1])){
				line[i] = '\0';
				*lineLength = i;

				break;
			}
		}
	}


	return(line);
}