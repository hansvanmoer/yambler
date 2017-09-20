#include "yambler_parser.h"

#include "yambler_buffer.h"

#include <iconv.h>

#include <errno.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INTERNAL_ENCODING_NAME "UCS-4-INTERNAL"
#define ENCODING_NAME_LENGTH_MAX 32
#define BYTES_PER_CHAR 4 
#define DEFAULT_BUFFER_SIZE 1024

struct yambler_input_buffer{
	struct yambler_byte_buffer in_buffer;
	struct yambler_char_buffer out_buffer;
	yambler_read read;
	yambler_read_binary read_binary;
	char encoding_name[ENCODING_NAME_LENGTH_MAX+1];
	iconv_t encoding_descriptor;
};

static yambler_status yambler_input_buffer_create_helper(yambler_input_buffer_p *buffer, size_t initial_size){
	yambler_input_buffer_p result = malloc(sizeof(struct yambler_input_buffer));
	if(result == NULL){
		return YAMBLER_ALLOC_ERROR;
	}
	yambler_status status = yambler_char_buffer_create(&result->out_buffer, initial_size);
	if(status){
		free(result);
		return status;
	}
	*buffer = result;
	return YAMBLER_OK;
}

yambler_status yambler_input_buffer_create(yambler_input_buffer_p *buffer, size_t initial_size, yambler_read read_callback){
	assert(buffer != NULL);
	yambler_input_buffer_p result;
	if(initial_size == 0){
		initial_size = DEFAULT_BUFFER_SIZE;
	}
	yambler_status status = yambler_input_buffer_create_helper(&result, initial_size);
	if(status){
		return status;
	}
	yambler_byte_buffer_create_unready(&result->in_buffer);
	result->read = read_callback;
	result->read_binary = NULL;
	result->encoding_name[0] = '\0';
	result->encoding_descriptor = (iconv_t)(-1);
	*buffer = result;
	return YAMBLER_OK;
}

yambler_status yambler_input_buffer_create_binary(yambler_input_buffer_p *buffer, size_t initial_size, yambler_read_binary read_callback, const char *encoding_name){
	assert(buffer != NULL);
	if(encoding_name && strlen(encoding_name) > ENCODING_NAME_LENGTH_MAX){
		return YAMBLER_BOUNDS_ERROR;
	}
	yambler_input_buffer_p result;
	yambler_status status = yambler_input_buffer_create_helper(&result, initial_size);
	if(status){
		return status;
	}
	yambler_byte_buffer_create_unready(&result->in_buffer);
	result->read = NULL;
	result->read_binary = read_callback;
	if(encoding_name){
		strcpy(result->encoding_name,encoding_name); 
	}else{
		result->encoding_name[0] = '\0';
	}
	result->encoding_descriptor = (iconv_t)(-1);
	*buffer = result;
	return YAMBLER_OK;
}

static yambler_status yambler_test_bom(yambler_input_buffer_p buffer){
	yambler_byte bom[4];
	size_t count;
	yambler_status status = ((*buffer->read_binary)(bom, 4, &count));
	
	if(status){
		return status;
	}

	const char *encoding_name;
	size_t bom_length;

	if(count < 2){
		bom_length = 0;
		encoding_name = "UTF_8";
	}else{
		if(count == 4 && bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF){
			encoding_name = "UTF-32BE";
			bom_length = 4;
		}else if(count == 4 && bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0x00){
			encoding_name = "UTF_32BE";
			bom_length = 0;
		}else if(count == 4 && bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00){
			encoding_name = "UTF_32LE";
			bom_length = 4;
		}else if(count == 4 && bom[1] == 0x00 && bom[2] == 0x00 && bom[3] == 0x00){
			encoding_name = "UTF_32LE";
			bom_length = 0;
		}else if(bom[0] == 0xFE && bom[1] == 0xFF){
			encoding_name = "UTF_16BE";
			bom_length = 2;
		}else if(bom[0] == 0x00){
			encoding_name = "UTF_16BE";
			bom_length = 0;
		}else if(bom[0] == 0xFF && bom[1] == 0xFE){
			encoding_name = "UTF_16LE";
			bom_length = 2;
		}else if(bom[1] == 0x00){
			encoding_name = "UTF_16_LE";
			bom_length = 0;
		}else if(count >= 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF){
			encoding_name = "UTF-8";
			bom_length = 3;
		}else{
			encoding_name = "UTF-8";
			bom_length = 0;
		}
	}
	if(bom_length != count){
		status = yambler_byte_buffer_put_all(&buffer->in_buffer, bom+bom_length, count - bom_length);
		if(status){
			return status;
		}
	}
	strcpy(buffer->encoding_name, encoding_name);
	return YAMBLER_OK;
}

yambler_status yambler_input_buffer_open(yambler_input_buffer_p buffer){
	if(!buffer->read && buffer->read_binary){
		yambler_status status = yambler_byte_buffer_create(&buffer->in_buffer, buffer->in_buffer.size * BYTES_PER_CHAR);
		if(status){
			return status;
		}
		if(buffer->encoding_name[0] == '\0'){
			if(yambler_test_bom(buffer)){
				return YAMBLER_INVALID_BOM;
			}
		}
		if(buffer->encoding_descriptor == (iconv_t)(-1)){
			buffer->encoding_descriptor = iconv_open(INTERNAL_ENCODING_NAME, buffer->encoding_name);
			if(buffer->encoding_descriptor == (iconv_t)(-1)){
				return YAMBLER_ENCODING_ERROR;
			}
		}
	}
	return YAMBLER_OK;
}

static yambler_status yambler_input_buffer_decode(yambler_input_buffer_p buffer){
	assert(buffer->read_binary != NULL);
	assert(buffer->in_buffer.length < buffer->in_buffer.size);
	assert(buffer->out_buffer.get != buffer->out_buffer.put);

	size_t count;
	yambler_status status = (*buffer->read_binary)(buffer->in_buffer.data + buffer->in_buffer.length,  buffer->in_buffer.size - buffer->in_buffer.length, &count);
	if(status){
		return status;
	}
	if(count == 0){
		return YAMBLER_EMPTY;
	}
	buffer->in_buffer.length+=count;

	yambler_char_buffer_shift(&buffer->out_buffer);

	char *in = (char *)buffer->in_buffer.data;
	size_t in_left = (buffer->in_buffer.length) * sizeof(yambler_byte);
	char *out = (char *)buffer->out_buffer.put;
	size_t out_left = (buffer->out_buffer.data + buffer->out_buffer.size - buffer->out_buffer.put) * sizeof(yambler_char);
	size_t conversion_count = 0;
	size_t result;
	while(out_left != 0 && (result = iconv(buffer->encoding_descriptor, &in, &in_left, &out, &out_left)) != ((size_t)-1)){
		conversion_count+=result;
	}
	if(result == (size_t)-1 && errno == EILSEQ){
		return YAMBLER_ENCODING_ERROR;
	}
	memmove(buffer->in_buffer.data, in, in_left);
	buffer->in_buffer.length = in_left / sizeof(yambler_byte);
	buffer->out_buffer.put += conversion_count;
	buffer->out_buffer.length += conversion_count;
	return YAMBLER_OK;
}

yambler_status yambler_input_buffer_reserve(yambler_input_buffer_p buffer, size_t length){
	if(buffer->out_buffer.length < length){
		if(buffer->out_buffer.size < length){
			yambler_status status = yambler_char_buffer_grow(&buffer->out_buffer, length);
			if(status){
				return status;
			}
		}
		yambler_status status = yambler_input_buffer_decode(buffer);
		if(status){
			return status;
		}
		if(buffer->out_buffer.length < length){
			return YAMBLER_EMPTY;
		}
	}else{
		return YAMBLER_OK;
	}
}

yambler_status yambler_input_buffer_get(yambler_input_buffer_p buffer, yambler_char *dest){
	if(buffer->out_buffer.length == 0){
		yambler_status status = yambler_input_buffer_reserve(buffer, 1);
		if(status){
			return status;
		}
	}
	*dest = *buffer->out_buffer.get++;
	--buffer->out_buffer.length;
	return YAMBLER_OK;
}

void yambler_input_buffer_destroy(yambler_input_buffer_p *buffer_p){
	assert(buffer_p != NULL);
	yambler_input_buffer_p buffer = *buffer_p;
	
	if(buffer->encoding_descriptor != (iconv_t)(-1)){
		iconv_close(buffer->encoding_descriptor);
	}
	if(yambler_byte_buffer_ready(&buffer->in_buffer)){
		yambler_byte_buffer_destroy(&buffer->in_buffer);
	}
	yambler_char_buffer_destroy(&buffer->out_buffer);
	free(buffer);
	
	*buffer_p = NULL;
}
