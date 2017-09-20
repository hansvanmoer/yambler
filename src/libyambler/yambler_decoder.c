#include "yambler_decoder.h"

#include "yambler_utility.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <iconv.h>

#define MIN_BUFFER_SIZE 4

#define DEFAULT_BUFFER_SIZE 4048

struct yambler_decoder{
	yambler_byte *buffer;
	size_t size;
	size_t length;

	enum yambler_encoding encoding;
	iconv_t descriptor;

	yambler_decoder_state read_state;
	yambler_decoder_open_callback open;
	yambler_decoder_read_callback read;
	yambler_decoder_close_callback close;
};

yambler_status yambler_decoder_create(yambler_decoder_p *result, size_t buffer_size, enum yambler_encoding encoding, yambler_decoder_read_callback read, yambler_decoder_state state, yambler_decoder_open_callback open, yambler_decoder_close_callback close){
	assert(result != NULL);
	
	if(buffer_size == 0){
		buffer_size = DEFAULT_BUFFER_SIZE;
	}else if(buffer_size < MIN_BUFFER_SIZE){
		return YAMBLER_BOUNDS_ERROR;
	}
	
	yambler_decoder_p decoder = malloc(sizeof(struct yambler_decoder));
	if(decoder == NULL){
		return YAMBLER_ALLOC_ERROR;
	}
	
	decoder->buffer = malloc(sizeof(yambler_byte) * buffer_size);
	if(decoder->buffer == NULL){
		free(decoder);
		return YAMBLER_ALLOC_ERROR;
	}
	decoder->size = buffer_size;
	decoder->length = 0;
	decoder->encoding = encoding;
	decoder->read_state = state;
	decoder->read = read;
	decoder->open = open;
	decoder->close = close;
	decoder->descriptor = (iconv_t)-1;
	*result = decoder;
	return YAMBLER_OK;
}


static yambler_status yambler_decoder_fill(yambler_decoder_p decoder){
	size_t remainder = decoder->size - decoder->length;
	yambler_byte *get = decoder->buffer + remainder;
	memmove(decoder->buffer, get, decoder->length);
	get = decoder->buffer;
	yambler_byte *put = get + decoder->length;
	if(decoder->read){
   		size_t total_count = 0;
		size_t count = 1;
		while(remainder != 0 && count != 0){
			size_t count;
			yambler_status status = (*decoder->read)(decoder->read_state, put, remainder, &count);
			if(status){
				return status;
			}
			remainder-=count;
			total_count+=count;
		}
		decoder->length+=total_count;
	}
	return YAMBLER_OK;
}

static yambler_status yambler_decoder_detect_encoding(yambler_decoder_p decoder, enum yambler_encoding *dest){
	yambler_status status = yambler_decoder_fill(decoder);
	if(status){
		return status;
	}
	enum yambler_encoding encoding;
	size_t bom_size;
	size_t length = decoder->length;
	yambler_byte *bom = decoder->buffer;
	if(length >= 4 && bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF){
		encoding = YAMBLER_ENCODING_UTF_32BE;
		bom_size = 4;
	}else if(length >= 4 && bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0x00){
		encoding = YAMBLER_ENCODING_UTF_32BE;
		bom_size = 0;
	}else if(length >= 4 && bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00){
		encoding = YAMBLER_ENCODING_UTF_32LE;
		bom_size = 4;
	}else if(length >= 4 && bom[1] == 0x00 && bom[2] == 0x00 && bom[3] == 0x00){
		encoding = YAMBLER_ENCODING_UTF_32LE;
		bom_size = 0;
	}else if(length >= 2 && bom[0] == 0xFE && bom[1] == 0xFF){
		encoding = YAMBLER_ENCODING_UTF_16BE;
		bom_size = 2;
	}else if(length >=2 && bom[0] == 0x00){
		encoding = YAMBLER_ENCODING_UTF_16BE;
		bom_size = 0;
	}else if(length >= 2 && bom[0] == 0xFF && bom[1] == 0xFE){
		encoding = YAMBLER_ENCODING_UTF_16LE;
		bom_size = 2;
	}else if(length >= 2 && bom[1] == 0x00){
		encoding = YAMBLER_ENCODING_UTF_16LE;
		bom_size = 0;
	}else{
		if(length >= 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF){
			bom_size = 3;
		}
		encoding = YAMBLER_ENCODING_UTF_8;
	}
	decoder->length-=bom_size;
	*dest = encoding;
	return YAMBLER_OK;
}

yambler_status yambler_decoder_open(yambler_decoder_p decoder){
	assert(decoder != NULL);
	
	if(decoder->open){
		yambler_status status = (*decoder->open)(&decoder->read_state);
		if(status){
			return status;
		}
	}
	enum yambler_encoding encoding = decoder->encoding;
	if(encoding == YAMBLER_ENCODING_DETECT){
		yambler_status status = yambler_decoder_detect_encoding(decoder, &encoding);
		if(status){
			if(decoder->close){
				(*decoder->close)(&decoder->read_state);
			}
			return status;
		}
	}
	iconv_t descriptor = iconv_open(YAMBLER_INTERNAL_ENCODING, yambler_encoding_name(encoding));
	if(descriptor == (iconv_t)-1){
		if(decoder->close){
			(*decoder->close)(&decoder->read_state);
		}
		return YAMBLER_ENCODING_ERROR;
	}
	decoder->length = 0;
	decoder->descriptor = descriptor;
	return YAMBLER_OK;
}

yambler_status yambler_decoder_decode(yambler_decoder_p decoder, yambler_char *buffer, size_t buffer_size, size_t *read_count){
	assert(decoder != NULL);
	assert(buffer != NULL);
	assert(buffer_size != 0);
	
	size_t out_remainder = buffer_size * sizeof(yambler_char);
	size_t total_count = 0;
	size_t count = 1;
	size_t in_remainder = 0;

	char *prev_out = (char*)buffer;

	while(out_remainder != 0 && count != 0){
		yambler_status status = yambler_decoder_fill(decoder);
		if(status){
			return status;
		}
		size_t new_in_remainder = decoder->length * sizeof(yambler_byte);
		if(in_remainder != 0 && in_remainder == new_in_remainder){
			return YAMBLER_ENCODING_ERROR;
		}
		in_remainder = new_in_remainder;
		char *in = decoder->buffer;
		char *out = prev_out;
		size_t result = iconv(decoder->descriptor, &in, &in_remainder, &out, &out_remainder);
		if(result == (size_t)-1 && errno == EILSEQ){
			return YAMBLER_ENCODING_ERROR;
		}
		decoder->length -= (in - (char*)&decoder->buffer) / sizeof(yambler_byte);
		count = (out - prev_out) / sizeof(yambler_char);
		total_count+=count;
		prev_out = out;
	}
	if(read_count){
		*read_count = total_count;
	}
	return YAMBLER_OK;
}

void yambler_decoder_close(yambler_decoder_p decoder){
	assert(decoder != NULL);
	assert(decoder->descriptor != (iconv_t)-1);
	
	iconv_close(decoder->descriptor);
	if(decoder->close){
		(*decoder->close)(&decoder->read_state);
	}
}

void yambler_decoder_destroy(yambler_decoder_p *src){
	assert(src != NULL);
	assert(*src != NULL);

	yambler_decoder_p decoder = *src;
	free(decoder->buffer);
	free(decoder);
	*src = NULL;
}
