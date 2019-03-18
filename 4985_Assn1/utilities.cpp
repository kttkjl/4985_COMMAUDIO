#include "utilities.h"

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: countActualBytes
--
--    DATE : FEB 08, 2019
--
--    REVISIONS :
--    		(FEB 08, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : int countActualBytes(char * buf, int len)
--
--    RETURNS : int
--				The total number of 'effective' bytes read from a given array of characters
--
--    NOTES :
--			Incoming WSARecv calls gives out how much bytes it has read from the socket, however, it counts the entire
--			size of the incoming buffer, but does not take into account the null bytes received as well.
----------------------------------------------------------------------------------------------------------------------*/
int countActualBytes(char * buf, int len) {
	int counter = 0;
	while (buf[counter] != '\0' && (counter < len)) {
		counter++;
	}
	return counter;
}

char * convertErrString(const char * string, unsigned int num) {
	char cstr[128];
	sprintf_s(cstr, "%s %u\n", string, num);
	return cstr;
}
