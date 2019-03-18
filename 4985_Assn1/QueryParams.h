#pragma once
#include <string.h>
#define INPUT_MAX_CHAR		128
#define OUTPUT_MAX_CHAR		128

typedef struct QueryParams {
	char addrStr[INPUT_MAX_CHAR]{ '\0' };
	char portStr[INPUT_MAX_CHAR]{ '\0' };
	char packetSizeStr[INPUT_MAX_CHAR]{ '\0' };
	char reqFilename[INPUT_MAX_CHAR]{ '\0' };
	bool stream = false;
} QueryParams, *LPQueryParams;

void clearInputs(LPQueryParams qp);
