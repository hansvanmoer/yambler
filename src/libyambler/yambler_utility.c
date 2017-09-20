#include "yambler_utility.h"

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
