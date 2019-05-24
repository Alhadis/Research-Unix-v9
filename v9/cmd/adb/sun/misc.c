/*
 * miscellany
 */

#include "defs.h"


/*
 * swap bytes in a long
 */

long
swal(l)
long l;
{

	return (((l >> 16) & 0xffff) | ((l & 0xffff) << 16));
}
