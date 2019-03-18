#pragma once
#include "QueryParams.h"
/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: clearInputs
--
--    DATE : FEB 10, 2019
--
--    REVISIONS :
--    		(FEB 10, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : void clearInputs()
--
--    RETURNS : void
--
--    NOTES :
--			clears all the input buffers filled by the windows dialogBox call on setting up the server, namely, the
--			PORT
--			INPUT BUFFER SIZE
----------------------------------------------------------------------------------------------------------------------*/
void clearInputs(LPQueryParams qp) {
	memset(qp->addrStr, '\0', INPUT_MAX_CHAR);
	memset(qp->portStr, '\0', INPUT_MAX_CHAR);
	memset(qp->packetSizeStr, '\0', INPUT_MAX_CHAR);
	memset(qp->reqFilename, '\0', INPUT_MAX_CHAR);
	qp->stream = false;
}
