#include "yambler_type.h"

const char *yambler_status_message(yambler_status status){
	switch(status){
	case YAMBLER_OK:
		return "ok";
	case YAMBLER_ERROR:
		return "unspecified error";
	case YAMBLER_ALLOC_ERROR:
		return "memory allocation failed";
	case YAMBLER_BOUNDS_ERROR:
		return "bounds checking failure";
	case YAMBLER_FULL:
		return "buffer is full";
	case YAMBLER_EMPTY:
		return "buffer is empty";
	case YAMBLER_INVALID_BOM:
		return "invalid byte order mark";
	case YAMBLER_ENCODING_ERROR:
		return "encoding error";
	default:
		return "unknown error";
	}
}
