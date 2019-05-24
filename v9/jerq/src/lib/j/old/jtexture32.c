#include <jerq.h>
jtexture32(r, map, f)
	Rectangle r;
	Texture32 *map;
{
	texture32(&display, r, map, f);
}
