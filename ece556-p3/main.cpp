// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.h"
#include "printStuff.h"
#include <string.h>

static int parseBinaryFlag(const char *arg, const char *prefix, int *valueOut)
{
	if (arg == NULL || prefix == NULL || valueOut == NULL) {
		return 0;
	}

	size_t prefixLength = strlen(prefix);
	if (strncmp(arg, prefix, prefixLength) != 0) {
		return 0;
	}

	if (arg[prefixLength] == '0' && arg[prefixLength + 1] == '\0') {
		*valueOut = 0;
		return 1;
	}

	if (arg[prefixLength] == '1' && arg[prefixLength + 1] == '\0') {
		*valueOut = 1;
		return 1;
	}

	return 0;
}

int main(int argc, char **argv)
{

 	if (argc != 5) {
 		printf("Usage : ./ROUTE.exe -d=<0|1> -n=<0|1> <input_benchmark_name> <output_file_name> \n");
 		return 1;
 	}

 	int status;
	int usePinOrdering = 0;
	int useNetOrderingAndRrr = 0;
	if (!parseBinaryFlag(argv[1], "-d=", &usePinOrdering) ||
		!parseBinaryFlag(argv[2], "-n=", &useNetOrderingAndRrr)) {
		printf("Usage : ./ROUTE.exe -d=<0|1> -n=<0|1> <input_benchmark_name> <output_file_name> \n");
		return 1;
	}

	char *inputFileName = argv[3];
 	char *outputFileName = argv[4];
	setRoutingMode(usePinOrdering, useNetOrderingAndRrr);

 	/// create a new routing instance
 	routingInst *rst = new routingInst;
	
 	/// read benchmark
 	status = readBenchmark(inputFileName, rst);
 	if (status == 0) {
 		printf("ERROR: reading input file \n");
 		return 1;
 	}
	
 	/// run actual routing
 	status = solveRouting(rst);
 	if (status == 0) {
 		printf("ERROR: running routing \n");
 		release(rst);
 		return 1;
 	}
	
 	/// write the result
 	status = writeOutput(outputFileName, rst);
 	if (status == 0) {
 		printf("ERROR: writing the result \n");
 		release(rst);
 		return 1;
 	}

    // DEBUG print statement
    // printRoutingInst(rst, TRUE);

    
 	release(rst);
 	delete rst;
 	printf("\nDONE!\n");	
 	return 0;
}
