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

	yambler_encoder_p encoder = malloc(sizeof(yambler_encoder_p));
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
	
	return YAMBLER_OK;
}

static void add_bom(yambler_encoder_p encoder){
   
	assert(encoder != NULL);
	assert(encoder->length == 0);

	yambler_byte *bom = encoder->buffer;
	size_t length;
	switch(encoder->encoding){
	case YAMBLER_ENCODING_UTF_32BE:
		bom[0] = 0xFE;
		bom[1] = 0xFF;
		bom[2] = 0x00;
		bom[3] = 0x00;
		length = 4;
		break;
	case YAMBLER_ENCODING_UTF_32LE:
		bom[0] = 0x00;
		bom[1] = 0x00;
		bom[2] = 0xFF;
		bom[3] = 0xFE;
		length = 4;
		break;
	case YAMBLER_ENCODING_UTF_16BE:
		bom[0] = 0xFE;
		bom[1] = 0xFF;
		length = 2;
		break;
	case YAMBLER_ENCODING_UTF_16LE:
		bom[0] = 0xFF;
		bom[1] = 0xFE;
		length = 2;
		break;
	case YAMBLER_ENCODING_UTF_8:
		bom[0] = 0xEF;
		bom[1] = 0xBB;
		bom[2] = 0xBF;
		length = 3;
		break;
	default:
		length = 0;
	}
	encoder->length = length;
}

yambler_status yambler_encoder_open(yambler_encoder_p encoder){
	assert(encoder != NULL);
	
	encoder->descriptor = iconv_open(YAMBLER_INTERNAL_ENCODING, yambler_encoding_name(encoder->encoding));
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
	if(encoder->write){
		size_t count = 1;
		size_t total_count = 0;
		yambler_byte *out = encoder->buffer;
		while(total_count != encoder->length && count != 0){
			yambler_status status = (*encoder->write)(encoder->write_state, out, encoder->size, &count);
			if(status){
				return status;
			}
			out+=count;
			total_count+=count;
		}
		encoder->length-=total_count;
		memmove(encoder->buffer, encoder->buffer + encoder->size - encoder->length, encoder->length * sizeof(yambler_byte));
	}
	return YAMBLER_OK;
}

yambler_status yambler_encoder_encode(yambler_encoder_p encoder, const yambler_char *buffer, size_t buffer_size, size_t *write_count){
	assert(encoder != NULL);
	assert(buffer != NULL);
	
	size_t in_remainder = buffer_size * sizeof(yambler_char);
	char *in = (char *)buffer; //NOTE: some versions of iconv take char ** instead of const char ** need to fix this
	char *prev_in = in;
	size_t total_count = 0;
	while(in_remainder != 0){
		size_t out_remainder = (encoder->size - encoder->length) * sizeof(yambler_byte);
		char *out = (char *)(encoder->buffer + encoder->length);
		size_t result = iconv(encoder->descriptor, &out, &out_remainder, &in, &in_remainder);
		if(result == (size_t)-1){
			if(errno == EILSEQ || errno == EINVAL){
				return YAMBLER_ENCODING_ERROR;
			}
		}
		size_t count = (prev_in - in) / sizeof(yambler_char);
		prev_in = in;
		encoder->length+=count;
		yambler_status status = yambler_encoder_flush(encoder);
		if(status){
			return status;
		}
		if(encoder->length != 0){
			return YAMBLER_ENCODING_ERROR;
		}
		total_count += count;
	}
	if(write_count){
		*write_count = total_count;
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
