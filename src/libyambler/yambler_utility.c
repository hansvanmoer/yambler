#include "yambler_utility.h"

#include <limits.h>
#include <stdint.h>

#if CHAR_BIT != 8

#error "unsupported char size: should be 8 bits"

#endif

static const union{
	char byte_value[4];
	uint32_t numeric_value;
} endianness_test = {{0x01, 0x02, 0x03, 0x04}};

const char *yambler_native_encoding_name(){
	if(endianness_test.numeric_value == 0x01020304ul){
		//big endian
		return "UTF-32BE";
	}else{
		//little endian
		return "UTF-32LE";
	}
}

const char *yambler_encoding_name(enum yambler_encoding encoding){
	switch(encoding){
	case YAMBLER_ENCODING_UTF_32BE:
		return "UTF-32BE";
	case YAMBLER_ENCODING_UTF_32LE:
		return "UTF-32LE";
	case YAMBLER_ENCODING_UTF_16BE:
		return "UTF-16BE";
	case YAMBLER_ENCODING_UTF_16LE:
		return "UTF-16LE";
	default:
		return "UTF-8";
	}
}
