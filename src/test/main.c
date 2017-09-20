#include "test.h"

#include <stdio.h>

int test_test(){
	return 0;
}

int main(int arg_count, const char **args){
	add_test("test", &test_test);
	return test_main(arg_count, args);
}
