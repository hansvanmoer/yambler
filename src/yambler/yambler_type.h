#ifndef YAMBLER_TYPE_H
#define YAMBLER_TYPE_H

#include <stdint.h>

typedef char yambler_byte;

typedef uint32_t yambler_char;

enum yambler_status_code{
	YAMBLER_OK = 0,
	YAMBLER_ERROR,
	YAMBLER_ALLOC_ERROR,
	YAMBLER_BOUNDS_ERROR,
	YAMBLER_FULL,
	YAMBLER_EMPTY,
	YAMBLER_INVALID_BOM,
	YAMBLER_ENCODING_ERROR
};

typedef enum yambler_status_code yambler_status;

#endif
