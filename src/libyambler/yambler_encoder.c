#include "yambler_encoder.h"

#include "yambler_utility.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <iconv.h>
#include <errno.h>

#define MIN_BUFFER_SIZE 4
#define DEFAULT_BUFFER_SIZE 4096

struct yambler_encoder{
	yambler_byte *buffer;
	size_t size;
	size_t length;
	
	enum yambler_encoding encoding;
	yambler_encoder_flag options;
	iconv_t descriptor;

	yambler_encoder_state write_state;
	yambler_encoder_write_callback write;
	yambler_encoder_open_callback open;
	yambler_encoder_close_callback close;
};

yambler_status yambler_encoder_create(yambler_encoder_p *dest, size_t buffer_size, enum yambler_encoding encoding, yambler_encoder_flag options, yambler_encoder_write_callback write, yambler_encoder_state state, yambler_encoder_open_callback open, yambler_encoder_close_callback close){
	assert(dest != NULL);
	
	if(buffer_size == 0){
		buffer_size = DEFAULT_BUFFER_SIZE;
	}else if(buffer_size < MIN_BUFFER_SIZE){
		return YAMBLER_BOUNDS_ERROR;
	}

	yambler_encoder_p encoder = malloc(sizeof(struct yambler_encoder));
	if(encoder == NULL){
		return YAMBLER_ALLOC_ERROR;
	}
	
	encoder->buffer = malloc(sizeof(yambler_byte) * buffer_size);
	if(encoder->buffer == NULL){
		free(encoder);
		return YAMBLER_ALLOC_ERROR;
	}
	encoder->size = buffer_size;
	encoder->length = 0;

	encoder->encoding = encoding;

	encoder->options = options;
	encoder->descriptor = (iconv_t)-1;
	
	encoder->write_state = state;
	encoder->write = write;
	encoder->open = open;
	encoder->close = close;

	*dest = encoder;
	
	return YAMBLER_OK;
}

static void add_bom(yambler_encoder_p encoder){
   
	assert(encoder != NULL);
	assert(encoder->length == 0);
	
	switch(encoder->encoding){
	case YAMBLER_ENCODING_UTF_32BE:
		encoder->buffer[0] = 0xFE;
		encoder->buffer[1] = 0xFF;
		encoder->buffer[2] = 0x00;
		encoder->buffer[3] = 0x00;
		encoder->length = 4;
		break;
	case YAMBLER_ENCODING_UTF_32LE:
		encoder->buffer[0] = 0x00;
		encoder->buffer[1] = 0x00;
		encoder->buffer[2] = 0xFF;
		encoder->buffer[3] = 0xFE;
		encoder->length = 4;
		break;
	case YAMBLER_ENCODING_UTF_16BE:
		encoder->buffer[0] = 0xFE;
		encoder->buffer[1] = 0xFF;
		encoder->length = 2;
		break;
	case YAMBLER_ENCODING_UTF_16LE:
		encoder->buffer[0] = 0xFF;
		encoder->buffer[1] = 0xFE;
		encoder->length = 2;
		break;
	case YAMBLER_ENCODING_UTF_8:
		encoder->buffer[0] = 0xEF;
		encoder->buffer[1] = 0xBB;
		encoder->buffer[2] = 0xBF;
		encoder->length = 3;
		break;
	default:
		encoder->length = 0;
	}
}

yambler_status yambler_encoder_open(yambler_encoder_p encoder){
	assert(encoder != NULL);
	
	encoder->descriptor = iconv_open(yambler_encoding_name(encoder->encoding), yambler_native_encoding_name());
	if(encoder->descriptor == (iconv_t)-1){
		return YAMBLER_ENCODING_ERROR;
	}

	if(encoder->open){
		yambler_status status = (*encoder->open)(&encoder->write_state);
		if(status){
			return status;
		}
	}

	if(encoder->options & YAMBLER_ENCODER_INCLUDE_BOM){
		add_bom(encoder);
	}
	return YAMBLER_OK;
}

static yambler_status yambler_encoder_flush(yambler_encoder_p encoder){
	assert(encoder != NULL);
	
	if(encoder->write && encoder->length != 0){
		size_t count;
		yambler_status status = (*encoder->write)(encoder->write_state, encoder->buffer, encoder->length, &count);
		if(status){
			return status;
		}else if(count != encoder->length){
			return YAMBLER_ERROR;
		}
		encoder->length = 0;
	}
	return YAMBLER_OK;
}

yambler_status yambler_encoder_encode(yambler_encoder_p encoder, const yambler_char *buffer, size_t buffer_size, size_t *write_count){
	assert(encoder != NULL);
	assert(buffer != NULL);

	size_t in_remainder = buffer_size * sizeof(yambler_char);
	char *in = (char *)buffer;
	
	while(1){
		char *out = (char *)(encoder->buffer + encoder->length);
		size_t out_remainder = (encoder->size - encoder->length) * sizeof(yambler_byte);
		
		size_t result = iconv(encoder->descriptor, &in, &in_remainder, &out, &out_remainder);
		if(result == (size_t)-1){
			if(errno == EINVAL || errno == EILSEQ){
				return YAMBLER_ENCODING_ERROR;
			}
		}
		encoder->length = encoder->size - out_remainder / sizeof(yambler_byte);

		yambler_status status = yambler_encoder_flush(encoder);
		if(status){
			return status;
		}
		if(in_remainder == 0){
			break;
		}
	}

	if(write_count){
		*write_count = in_remainder / sizeof(yambler_char);
	}
	
	return YAMBLER_OK;
}

void yambler_encoder_close(yambler_encoder_p encoder){
	assert(encoder != NULL);
	
	if(encoder->close){
		(*encoder->close)(&encoder->write_state);
	}
	iconv_close(encoder->descriptor);
	encoder->length = 0;
	encoder->descriptor = (iconv_t)-1;
}

void yambler_encoder_destroy(yambler_encoder_p *src){
	assert(src != NULL);
	assert(*src != NULL);

	yambler_encoder_p encoder = *src;
	free(encoder->buffer);
	free(encoder);
	*src = NULL;
}
