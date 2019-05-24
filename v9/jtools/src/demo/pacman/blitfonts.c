#include <jerq.h>

short Bits24[] = {

	0x0 , 0x0 , 0x1800 , 0x0 , 0x0 , 0x0 ,
	0x18 , 0x0 , 0x1800 , 0x0 , 

	0x0 , 0x0 , 0x1f8 , 0x0 , 0xff , 0x0 ,
	0x0 , 0x1800 , 0x0 , 0x0 , 

	0x0 , 0x18 , 0x0 , 0x1800 , 0x0 , 0x0 ,
	0x0 , 0x7fe , 0x0 , 0x0 , 

	0x0 , 0x0 , 0x1800 , 0x0 , 0x0 , 0x0 ,
	0x18 , 0x0 , 0x1800 , 0x0 , 

	0x0 , 0x0 , 0x1fff , 0x800f , 0xfff0 , 0x0 ,
	0x0 , 0x1800 , 0x0 , 0x0 , 

	0x0 , 0x18 , 0x0 , 0x1800 , 0x0 , 0x0 ,
	0x0 , 0x3fff , 0xc000 , 0x3f , 

	0x0 , 0x0 , 0x1800 , 0x0 , 0x0 , 0x0 ,
	0x18 , 0x0 , 0x1800 , 0x0 , 

	0x0 , 0x0 , 0x3fff , 0xc0ff , 0xfc00 , 0x0 ,
	0x0 , 0x1800 , 0x0 , 0x0 , 

	0x0 , 0x18 , 0x0 , 0x1800 , 0x0 , 0x0 ,
	0x0 , 0x7fff , 0xe000 , 0x7fff , 

	0x0 , 0x0 , 0x1800 , 0x0 , 0x0 , 0x0 ,
	0x18 , 0x0 , 0x1800 , 0x0 , 

	0x0 , 0x0 , 0x7fff , 0xe0fe , 0x1 , 0x0 ,
	0x0 , 0x1800 , 0x0 , 0x0 , 

	0x0 , 0x18 , 0x0 , 0x1800 , 0x0 , 0x0 ,
	0x0 , 0xffff , 0xf0ff , 0xffff , 

	0x0 , 0x0 , 0x1800 , 0x0 , 0x0 , 0x0 ,
	0x38 , 0x0 , 0x1c00 , 0x0 , 

	0x0 , 0x0 , 0xffff , 0xf080 , 0x3ff , 0x0 ,
	0x0 , 0x1800 , 0x0 , 0x0 , 

	0x0 , 0x70 , 0x0 , 0xe00 , 0x0 , 0x0 ,
	0x1800 , 0xffff , 0xf0ff , 0xffc0 , 

	0x0 , 0x0 , 0x1800 , 0x0 , 0x0 , 0x0 ,
	0xe0 , 0x0 , 0x700 , 0x0 , 

	0x0 , 0x3c00 , 0xffff , 0xf007 , 0xffff , 0xffff ,
	0xff00 , 0x1800 , 0x1 , 0xffff , 

	0x8000 , 0xffc0 , 0x0 , 0x3ff , 0x3333 , 0x3300 ,
	0x7e00 , 0xffff , 0xf0ff , 0xe00f , 

	0xffff , 0xff00 , 0x1800 , 0x3 , 0xffff , 0xc000 ,
	0xff80 , 0x0 , 0x1ff , 0xcccc , 

	0xcc00 , 0x7e00 , 0xffff , 0xf0ff , 0xffff , 0x0 ,
	0x0 , 0x1800 , 0x7 , 0x0 , 

	0xe000 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x3c00 , 0x7fff , 0xe0f0 , 0xfff , 

	0x0 , 0x0 , 0x1800 , 0xe , 0x0 , 0x7000 ,
	0x0 , 0x0 , 0x0 , 0x0 , 

	0x0 , 0x1800 , 0x7fff , 0xe0ff , 0xfff0 , 0x0 ,
	0x0 , 0x1800 , 0x1c , 0x0 , 

	0x3800 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x3fff , 0xc01f , 0xffff , 

	0x0 , 0x0 , 0x1800 , 0x18 , 0x0 , 0x1800 ,
	0x0 , 0x0 , 0x0 , 0x0 , 

	0x0 , 0x0 , 0x3fff , 0xc0ff , 0xf83f , 0x0 ,
	0x0 , 0x1800 , 0x18 , 0x0 , 

	0x1800 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x1fff , 0x80ff , 0xffff , 

	0x0 , 0x0 , 0x1800 , 0x18 , 0x0 , 0x1800 ,
	0x0 , 0x0 , 0x0 , 0x0 , 

	0x0 , 0x0 , 0x7fe , 0xfc , 0x3fff , 0x0 ,
	0x0 , 0x1800 , 0x18 , 0x0 , 

	0x1800 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x1f8 , 0xff , 0xfffc , 

	0x0 , 0x0 , 0x1800 , 0x18 , 0x0 , 0x1800 ,
	0x0 , 0x0 , 0x0 , 0x0 , 

	0x0 , 0x0 , 0x0 , 0x7f , 0xffff , 0x0 ,
	0x0 , 0x1800 , 0x18 , 0x0 , 

	0x1800 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff , 0xfe7f , 

	0x0 , 0x0 , 0x1800 , 0x18 , 0x0 , 0x1800 ,
	0x0 , 0x0 , 0x0 , 0x0 , 

	0x0 , 0x0 , 0x0 , 0xff , 0xffff , 0x0 ,
	0x0 , 0x1800 , 0x18 , 0x0 , 

	0x1800 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xfe , 0x7fff

};

short Bits40[] = {

	0x0 , 0xff00 , 0x0 , 0xff , 0x0 ,
	0x0 , 0xff00 , 0x0 , 0xff , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xff , 0x0 ,
	0x0 , 0xff00 , 0x0 , 0xff , 0x0 ,
	0x0 , 

	0xff00 , 0x0 , 0xff , 0x0 , 0x0 ,
	0xff00 , 0x0 , 0xff , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x55 , 0x0 , 0x0 ,
	0xaa00 , 0x0 , 0x0 , 0x700 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x1800 , 

	0x0 , 0xff , 0x0 , 0x0 , 0xff00 ,
	0x0 , 0xff , 0x0 , 0x0 , 0x0 ,
	0xcc00 , 0x0 , 0x0 , 0xf , 0xfff0 ,
	0x0 , 0xfff , 0xf000 , 0xf , 0xfff0 ,
	0x0 , 0xfff , 0xf000 , 0xe , 0x70 ,
	0x0 , 

	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0xfff , 0xf000 , 0xf , 0xfff0 , 0x0 ,
	0xfff , 0xf000 , 0xf , 0xfff0 , 0x0 ,
	0xfff , 0xf000 , 0xf , 0xfff0 , 0x0 ,
	0xfff , 0xf000 , 0x0 , 0x0 , 0x0 ,
	0xaaa , 

	0xa000 , 0xa , 0xaaa0 , 0x0 , 0x0 ,
	0x1f80 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x1800 , 0x0 , 0xfff ,
	0xf000 , 0xf , 0xfff0 , 0x0 , 0xfff ,
	0xf000 , 0x4e00 , 0xcc , 0x4e00 , 0x0 ,
	0x0 , 

	0x3f , 0xfffc , 0x0 , 0x3fff , 0xfc00 ,
	0x3f , 0xfffc , 0x0 , 0x3fff , 0xfc00 ,
	0x3e , 0x7c , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x3fff , 0xfc00 ,
	0x3f , 0xfffc , 0x0 , 0x3fff , 0xfc00 ,
	0x3f , 

	0xfffc , 0x0 , 0x3fff , 0xfc00 , 0x3f ,
	0xfffc , 0x0 , 0x3fff , 0xfc00 , 0x0 ,
	0x0 , 0x0 , 0x1555 , 0x5400 , 0x2a ,
	0xaaa8 , 0x0 , 0x0 , 0x1fc0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x1800 , 

	0x0 , 0x3fff , 0xfc00 , 0x3f , 0xfffc ,
	0x0 , 0x3fff , 0xfc00 , 0x0 , 0x100 ,
	0x0 , 0x0 , 0x0 , 0x7f , 0xfffe ,
	0x0 , 0x7fff , 0xfe00 , 0x7f , 0xfffe ,
	0x0 , 0x7fff , 0xfe00 , 0x7e , 0x7e ,
	0x0 , 

	0x6000 , 0x600 , 0x0 , 0x0 , 0x0 ,
	0x7fff , 0xfe00 , 0x7f , 0xfffe , 0x0 ,
	0x7fff , 0xfe00 , 0x7f , 0xfffe , 0x0 ,
	0x7fff , 0xfe00 , 0x7f , 0xfffe , 0x0 ,
	0x7fff , 0xfe00 , 0x0 , 0x0 , 0x0 ,
	0x2aaa , 

	0xaa00 , 0x2a , 0xaaaa , 0x0 , 0x0 ,
	0x37c0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x1800 , 0x0 , 0x7fff ,
	0xfe00 , 0x7f , 0xfffe , 0x0 , 0x7fff ,
	0xfe00 , 0x41 , 0x100 , 0x0 , 0x0 ,
	0x0 , 

	0x1ff , 0xffff , 0x8001 , 0xffff , 0xff80 ,
	0x1ff , 0xffff , 0x8001 , 0xffff , 0xff80 ,
	0x1ff , 0xff , 0x8001 , 0xe000 , 0x780 ,
	0x0 , 0x0 , 0x1 , 0xffff , 0xff80 ,
	0x1ff , 0xffff , 0x8001 , 0xffff , 0xff80 ,
	0x1ff , 

	0xffff , 0x8001 , 0xffff , 0xff80 , 0x1ff ,
	0xffff , 0x8001 , 0xffff , 0xff80 , 0x0 ,
	0x0 , 0x1 , 0x5555 , 0x5500 , 0xaa ,
	0xaaaa , 0x8000 , 0x0 , 0xec00 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x1800 , 

	0x1 , 0xffff , 0xff80 , 0x1ff , 0xffff ,
	0x8001 , 0xffff , 0xff80 , 0x8b4d , 0x0 ,
	0xcc00 , 0x0 , 0x0 , 0x3ff , 0xffff ,
	0xc003 , 0xffff , 0xffc0 , 0x3ff , 0xffff ,
	0xc003 , 0xffff , 0xff80 , 0x3ff , 0xff ,
	0xc003 , 

	0xf000 , 0xfc0 , 0x0 , 0x0 , 0x3 ,
	0xffff , 0xffc0 , 0x3ff , 0xffff , 0xc001 ,
	0xffff , 0xffc0 , 0x3ff , 0xffff , 0xc003 ,
	0xffff , 0xffc0 , 0x3ff , 0xffff , 0xc003 ,
	0xffff , 0xffc0 , 0x0 , 0x0 , 0x2 ,
	0xaaaa , 

	0xaa80 , 0x2aa , 0xaaaa , 0x8000 , 0x1 ,
	0x9800 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x1800 , 0x3 , 0xffff ,
	0xffc0 , 0x3ff , 0xffff , 0xc003 , 0xffff ,
	0xffc0 , 0x4a00 , 0x2 , 0x0 , 0x0 ,
	0x0 , 

	0x7ff , 0xffff , 0xe007 , 0xffff , 0xffe0 ,
	0x7ff , 0xffff , 0xe007 , 0xffff , 0xff00 ,
	0x7ff , 0xff , 0xe007 , 0xf000 , 0xfe0 ,
	0x0 , 0x0 , 0x7 , 0xffff , 0xffe0 ,
	0x7ff , 0xffff , 0xe000 , 0xffff , 0xffe0 ,
	0x7ff , 

	0xffff , 0xe007 , 0xffff , 0xffe0 , 0x7ff ,
	0xffff , 0xe007 , 0xffff , 0xffe0 , 0x0 ,
	0x0 , 0x5 , 0x5555 , 0x5540 , 0x2aa ,
	0xaaaa , 0xa000 , 0x7 , 0x3000 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x600 ,
	0x1800 , 

	0x6007 , 0x1ff , 0x80e0 , 0x7ff , 0xffff ,
	0xe007 , 0xffff , 0xffe0 , 0x100 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xfff , 0xffff ,
	0xf00f , 0xffff , 0xfff0 , 0xfff , 0xffff ,
	0xf00f , 0xffff , 0xfe00 , 0xfff , 0xff ,
	0xf00f , 

	0xf800 , 0x1ff0 , 0xc00 , 0x0 , 0x300f ,
	0xffff , 0xfff0 , 0xfff , 0xffff , 0xf000 ,
	0x7fff , 0xfff0 , 0xfff , 0xffff , 0xf00f ,
	0xffff , 0xfff0 , 0xfff , 0xffff , 0xf00f ,
	0xffff , 0xfff0 , 0x0 , 0x0 , 0xa ,
	0xaaaa , 

	0xaaa0 , 0xaaa , 0xaaaa , 0xa000 , 0xc ,
	0x3000 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x300 , 0x1800 , 0xc00e , 0x7cff ,
	0x3e70 , 0xfff , 0xffff , 0xf00f , 0xffff ,
	0xfff0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xfff , 0xffff , 0xf00f , 0xffff , 0xfff0 ,
	0xfff , 0xffff , 0xf00f , 0xffff , 0xfc00 ,
	0xfff , 0x81ff , 0xf00f , 0xf800 , 0x1ff0 ,
	0xe00 , 0x0 , 0x700f , 0xffff , 0xfff0 ,
	0xfff , 0xffff , 0xf000 , 0x3fff , 0xfff0 ,
	0xfff , 

	0xffff , 0xf00f , 0xffff , 0xfff0 , 0xfff ,
	0xffff , 0xf00f , 0xffff , 0xfff0 , 0x0 ,
	0x0 , 0x5 , 0x5555 , 0x5550 , 0xaaa ,
	0xaaaa , 0xa000 , 0x38 , 0x6000 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x180 ,
	0x1801 , 

	0x800c , 0x7c7e , 0x3e30 , 0xfff , 0xffff ,
	0xf00f , 0xffff , 0xfff0 , 0x0 , 0x0 ,
	0x600 , 0x0 , 0x0 , 0x1fff , 0xffff ,
	0xf81f , 0xffff , 0xfff8 , 0x1fff , 0xffff ,
	0xf81f , 0xffff , 0xf800 , 0x1fff , 0x81ff ,
	0xf81f , 

	0xfc00 , 0x3ff8 , 0x1f00 , 0x0 , 0xf81f ,
	0xffff , 0xfff8 , 0x1fff , 0xffff , 0xf800 ,
	0x1fff , 0xfff8 , 0x1fff , 0xffff , 0xf81f ,
	0xffff , 0xfff8 , 0x1fff , 0xffff , 0xf81f ,
	0xffff , 0xfff8 , 0x0 , 0x0 , 0xa ,
	0xaaaa , 

	0xaaa8 , 0xaaa , 0xaaaa , 0xa800 , 0x60 ,
	0x6000 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0xc0 , 0x1803 , 0x1c , 0xfe7e ,
	0x7f38 , 0x1fff , 0xffff , 0xf81f , 0xffff ,
	0xfff8 , 0x2 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x3fff , 0xffff , 0xfc3f , 0xffff , 0xfffc ,
	0x3fff , 0xffff , 0xf83f , 0xffff , 0xf000 ,
	0x3fff , 0x81ff , 0xfc3f , 0xfc00 , 0x3ffc ,
	0x3f80 , 0x1 , 0xfc3f , 0xffff , 0xfffc ,
	0x1fff , 0xffff , 0xfc00 , 0xfff , 0xfffc ,
	0x3fff , 

	0xffff , 0xfc3f , 0xffff , 0xfffc , 0x3fff ,
	0xffff , 0xfc3f , 0xf01f , 0xf80c , 0x0 ,
	0x0 , 0x15 , 0x5555 , 0x5554 , 0x2aaa ,
	0xaaaa , 0xa800 , 0xc0 , 0x6000 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x60 ,
	0x1806 , 

	0x3c , 0xfe7e , 0x7f3c , 0x301f , 0xf80f ,
	0xfc3f , 0xffff , 0xfffc , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x3fff , 0xffff ,
	0xfc3f , 0xffff , 0xfffc , 0x3fff , 0xffff ,
	0xe03f , 0xffff , 0xe000 , 0x3fff , 0x81ff ,
	0xfc3f , 

	0xfe00 , 0x7ffc , 0x3fc0 , 0x3 , 0xfc3f ,
	0xffff , 0xfffc , 0x7ff , 0xffff , 0xfc00 ,
	0x7ff , 0xfffc , 0x3fff , 0xffff , 0xfc3f ,
	0xffff , 0xfffc , 0x3fff , 0xffff , 0xfc3f ,
	0xe00f , 0xf00c , 0x0 , 0x0 , 0x2a ,
	0xaaaa , 

	0xaaa8 , 0x2aaa , 0xaaaa , 0xa800 , 0x180 ,
	0x6000 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x30 , 0x180c , 0x3c , 0xfe7e ,
	0x7f3c , 0x200f , 0xf007 , 0xfc3f , 0x1ff ,
	0x80fc , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfffe ,
	0x7fff , 0xffff , 0x807f , 0xffff , 0xc000 ,
	0x7fff , 0xc3ff , 0xfe7f , 0xfe00 , 0x7ffe ,
	0x7fe0 , 0x7 , 0xfe7f , 0xffff , 0xfffe ,
	0x1ff , 0xffff , 0xfe00 , 0x3ff , 0xfffe ,
	0x7fff , 

	0xffff , 0xfe7f , 0xffff , 0xfffe , 0x7fff ,
	0xffff , 0xfe7f , 0xc007 , 0xe006 , 0x0 ,
	0x0 , 0x55 , 0x5555 , 0x5554 , 0x2aaa ,
	0xaaaa , 0xaa00 , 0x300 , 0x6000 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x18 ,
	0x1818 , 

	0x7c , 0x7c7e , 0x3e3e , 0x6007 , 0xe003 ,
	0xfe7e , 0xff , 0x7e , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x7fff , 0xffff ,
	0xfe7f , 0xffff , 0xfffe , 0x7fff , 0xfffe ,
	0x7f , 0xffff , 0x8000 , 0x7fff , 0xc3ff ,
	0xfe7f , 

	0xff00 , 0xfffe , 0x7ff0 , 0xf , 0xfe7f ,
	0xffff , 0xfffe , 0x7f , 0xffff , 0xfe00 ,
	0x1ff , 0xfffe , 0x7fff , 0xffff , 0xfe7f ,
	0xffff , 0xfffe , 0x7fff , 0xffff , 0xfe7f ,
	0xc007 , 0xe002 , 0x1ff , 0xff , 0x802a ,
	0xaa , 

	0x2a , 0x2a00 , 0xaa00 , 0x2a00 , 0x600 ,
	0x6000 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0xc , 0x30 , 0x7c , 0x387e ,
	0x1c3e , 0x4007 , 0xe003 , 0xfe7c , 0x7e ,
	0x3e , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfffe ,
	0x7fff , 0xfff8 , 0x7f , 0xffff , 0x0 ,
	0x7fff , 0xc3ff , 0xfe7f , 0xff00 , 0xfffe ,
	0x7ff8 , 0x1f , 0xfe7f , 0xffff , 0xfffe ,
	0x1f , 0xffff , 0xfe00 , 0xff , 0xfffe ,
	0x7fff , 

	0xffff , 0xfe7f , 0xffff , 0xfffe , 0x7fff ,
	0xffff , 0xfe7f , 0xc1c7 , 0xe0e2 , 0x3ff ,
	0x81ff , 0xc054 , 0x54 , 0x14 , 0x2800 ,
	0x2a00 , 0x2a00 , 0xc00 , 0x6000 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x7c , 0x7e , 0x3e , 0x4707 , 0xe383 ,
	0xfefc , 0x7e , 0x3f , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x7fff , 0xffff ,
	0xfe7f , 0xffff , 0xfff0 , 0x7fff , 0xffe0 ,
	0x7f , 0xfffe , 0x0 , 0x7fff , 0xc3ff ,
	0xfe7f , 

	0xff81 , 0xfffe , 0x7ffc , 0x3f , 0xfe0f ,
	0xffff , 0xfffe , 0x7 , 0xffff , 0xfe00 ,
	0x7f , 0xfffe , 0x7fff , 0xffff , 0xfe7f ,
	0xffff , 0xfffe , 0x7fff , 0xffff , 0xfe7f ,
	0xc3e7 , 0xe1f2 , 0x3c7 , 0x81e3 , 0xc028 ,
	0x382a , 

	0x1c2a , 0x2838 , 0x2a1c , 0x2a00 , 0x1800 ,
	0x3000 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x7c , 0x7e ,
	0x3e , 0x4f87 , 0xe7c3 , 0xfefc , 0x7e ,
	0x3f , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xffff , 0xffff , 0xffff , 0xffff , 0xff00 ,
	0xffff , 0xff80 , 0xff , 0xfffc , 0x0 ,
	0xffff , 0xe7ff , 0xffff , 0xff81 , 0xffff ,
	0xfffe , 0x7f , 0xff00 , 0xffff , 0xffff ,
	0x1 , 0xffff , 0xff00 , 0x3f , 0xffff ,
	0xffff , 

	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xc7f7 , 0xe3fb , 0x383 ,
	0x81c1 , 0xc054 , 0x7c54 , 0x3e15 , 0xa87c ,
	0x2a3e , 0x2a00 , 0x1800 , 0x3000 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x7c , 0x7e , 0x3e , 0xdfc7 , 0xefe3 ,
	0xfffc , 0x387e , 0x1c3f , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xf000 , 0xffff , 0xfe00 ,
	0xff , 0xfff8 , 0x0 , 0xffff , 0xe7ff ,
	0xffff , 

	0xffc3 , 0xffff , 0xffff , 0xff , 0xff00 ,
	0xfff , 0xffff , 0x0 , 0x7fff , 0xff00 ,
	0x1f , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xc7f7 , 0xe3fb , 0x383 , 0x81c1 , 0xc0a8 ,
	0x7c2a , 

	0x3e2a , 0xa87c , 0x2a3e , 0x2a00 , 0x3000 ,
	0x3000 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xfe , 0xff ,
	0x7f , 0xdfc7 , 0xefe3 , 0xfffc , 0x7c7e ,
	0x3e3f , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xffff , 0xffff , 0xffff , 0xffff , 0x0 ,
	0xffff , 0xf800 , 0xff , 0xfff0 , 0x0 ,
	0xffff , 0xe7ff , 0xffff , 0xffc3 , 0xffff ,
	0xffff , 0x81ff , 0xff00 , 0xff , 0xffff ,
	0x0 , 0x1fff , 0xff00 , 0xf , 0xffff ,
	0xffff , 

	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xc7f7 , 0xe3fb , 0x383 ,
	0x81c1 , 0xc054 , 0x7c54 , 0x3e15 , 0xa87c ,
	0x2a3e , 0x2a00 , 0x3000 , 0x1800 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xff , 0x1ff , 0x80ff , 0xdfc7 , 0xefe3 ,
	0xfffc , 0xfe7e , 0x7f3f , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xffff , 0xffff ,
	0xffff , 0xfff0 , 0x0 , 0xffff , 0xf000 ,
	0xff , 0xffe0 , 0x0 , 0xffff , 0xe7ff ,
	0xffff , 

	0xffe7 , 0xffff , 0xffff , 0xc3ff , 0xff00 ,
	0xf , 0xffff , 0x0 , 0xfff , 0xff00 ,
	0x7 , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0xe7ff , 0xffff ,
	0xc3e7 , 0xe1f3 , 0x3c7 , 0x81e3 , 0xc0a8 ,
	0x382a , 

	0x1c2a , 0xa838 , 0x2a1c , 0x2a00 , 0x3000 ,
	0xc00 , 0x0 , 0x0 , 0x0 , 0x18 ,
	0x0 , 0xfff0 , 0xf , 0xffff , 0xffff ,
	0xffff , 0xcf87 , 0xe7c3 , 0xfffc , 0xfe7e ,
	0x7f3f , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xffff , 0xffff , 0xffff , 0xfff0 , 0x0 ,
	0xffff , 0xf000 , 0xff , 0xffe0 , 0x0 ,
	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xe7ff , 0xff00 , 0xf , 0xffff ,
	0x0 , 0xfff , 0xff00 , 0x7 , 0xffff ,
	0xffff , 

	0xe7ff , 0xffff , 0xffe7 , 0xffff , 0xffff ,
	0xc3ff , 0xffff , 0xc1c7 , 0xe0e3 , 0x3ff ,
	0x81ff , 0xc054 , 0x54 , 0x15 , 0xa800 ,
	0x2a00 , 0x2a00 , 0xfc00 , 0x3f00 , 0xffff ,
	0xffff , 0xff00 , 0x3c , 0x0 , 0xfff0 ,
	0xf , 

	0xffff , 0xffff , 0xffff , 0xc707 , 0xe383 ,
	0xfffc , 0xfe7e , 0x7f3f , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xffff , 0xffff ,
	0xffff , 0xffff , 0x0 , 0xffff , 0xf800 ,
	0xff , 0xfff0 , 0x0 , 0xffff , 0xffff ,
	0xffff , 

	0xffff , 0xffff , 0xffff , 0xffff , 0xff00 ,
	0xff , 0xffff , 0x0 , 0x1fff , 0xff00 ,
	0xf , 0xffff , 0xffff , 0xe7ff , 0xffff ,
	0xffc3 , 0xffff , 0xffff , 0x81ff , 0xffff ,
	0xc007 , 0xe003 , 0x1ff , 0xff , 0x80aa ,
	0xaa , 

	0x2a , 0xaa00 , 0xaa00 , 0x2a01 , 0xfe00 ,
	0x7f80 , 0xffff , 0xffff , 0xff00 , 0x7e ,
	0x0 , 0x0 , 0x0 , 0xff , 0xffff ,
	0xffff , 0xc007 , 0xe003 , 0xfffc , 0x7c7e ,
	0x3e3f , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xffff , 0xffff , 0xffff , 0xffff , 0xf000 ,
	0xffff , 0xfe00 , 0xff , 0xfff8 , 0x0 ,
	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xff00 , 0xfff , 0xffff ,
	0x0 , 0x7fff , 0xff00 , 0x1f , 0xffff ,
	0xffff , 

	0xe7ff , 0xffff , 0xffc3 , 0xffff , 0xffff ,
	0xff , 0xffff , 0xc007 , 0xe003 , 0x0 ,
	0x0 , 0x55 , 0x5555 , 0x5555 , 0xaaaa ,
	0xaaaa , 0xaa03 , 0xff00 , 0xffc0 , 0xffff ,
	0xffff , 0xff00 , 0xff , 0x0 , 0x0 ,
	0x0 , 

	0xff , 0xffff , 0xffff , 0xc007 , 0xe003 ,
	0xfffe , 0x7cff , 0x3e7f , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xff00 , 0xffff , 0xff80 ,
	0xff , 0xfffc , 0x0 , 0xffff , 0xffff ,
	0xffff , 

	0xffff , 0xffff , 0xffff , 0xffff , 0xff00 ,
	0xffff , 0xffff , 0x1 , 0xffff , 0xff00 ,
	0x3f , 0xffff , 0xffff , 0xe7ff , 0xffff ,
	0xff81 , 0xffff , 0xfffe , 0x7f , 0xffff ,
	0xe00f , 0xf007 , 0x0 , 0x0 , 0xaa ,
	0xaaaa , 

	0xaaaa , 0xaaaa , 0xaaaa , 0xaa07 , 0xff81 ,
	0xffe0 , 0xffff , 0xffff , 0xff00 , 0x1ff ,
	0x8000 , 0x0 , 0x0 , 0xff , 0xffff ,
	0xffff , 0xe00f , 0xf007 , 0xffff , 0x1ff ,
	0x80ff , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfff0 ,
	0x7fff , 0xffe0 , 0x7f , 0xfffe , 0x0 ,
	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfffe ,
	0x7fff , 0xffff , 0xfe0f , 0xffff , 0xfffe ,
	0x7 , 0xffff , 0xfe00 , 0x7f , 0xfffe ,
	0x7fff , 

	0xc3ff , 0xfe7f , 0xff81 , 0xfffe , 0x7ffc ,
	0x3f , 0xfeff , 0xf01f , 0xf80f , 0x0 ,
	0x0 , 0x55 , 0x5555 , 0x5555 , 0xaaaa ,
	0xaaaa , 0xaa0f , 0xffc3 , 0xfff0 , 0x7fff ,
	0xffff , 0xfe00 , 0x3ff , 0xc000 , 0x0 ,
	0x0 , 

	0xff , 0xffff , 0xffff , 0xf01f , 0xf80f ,
	0xffff , 0xffff , 0xffff , 0x0 , 0x0 ,
	0x64 , 0x2b00 , 0xd0 , 0x7fff , 0xffff ,
	0xfe7f , 0xffff , 0xfffe , 0x7fff , 0xfff8 ,
	0x7f , 0xffff , 0x0 , 0x7fff , 0xffff ,
	0xfe7f , 

	0xffff , 0xfffe , 0x7fff , 0xffff , 0xfe7f ,
	0xffff , 0xfffe , 0x1f , 0xffff , 0xfe00 ,
	0xff , 0xfffe , 0x7fff , 0xc3ff , 0xfe7f ,
	0xff00 , 0xfffe , 0x7ff8 , 0x1f , 0xfeff ,
	0xffff , 0xffff , 0x0 , 0x0 , 0xaa ,
	0x2a2a , 

	0x2a2a , 0xaaaa , 0xaaaa , 0xaa0f , 0xffc3 ,
	0xfff0 , 0x7fff , 0xffff , 0xfe00 , 0x7ff ,
	0xe000 , 0x0 , 0x0 , 0xff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0x0 , 0x0 , 0x52 , 0x0 ,
	0xd052 , 

	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfffe ,
	0x7fff , 0xfffe , 0x7f , 0xffff , 0x8000 ,
	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfffe ,
	0x7fff , 0xffff , 0xfe7f , 0xffff , 0xfffe ,
	0x7f , 0xffff , 0xfe00 , 0x1ff , 0xfffe ,
	0x7fff , 

	0xc3ff , 0xfe7f , 0xff00 , 0xfffe , 0x7ff0 ,
	0xf , 0xfeff , 0xffff , 0xffff , 0x0 ,
	0x0 , 0x54 , 0x1414 , 0x1415 , 0xaaaa ,
	0xaaaa , 0xaa0f , 0xffc3 , 0xfff0 , 0x7fff ,
	0xffff , 0xfe00 , 0xfff , 0xf000 , 0xc ,
	0x30 , 

	0xff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0x0 , 0x0 ,
	0x0 , 0xd8 , 0x5200 , 0x7fff , 0xffff ,
	0xfe7f , 0xffff , 0xfffe , 0x7fff , 0xffff ,
	0x807f , 0xffff , 0xc000 , 0x7fff , 0xffff ,
	0xfe7f , 

	0xffff , 0xfffe , 0x7fff , 0xffff , 0xfe7f ,
	0xffff , 0xfffe , 0x1ff , 0xffff , 0xfe00 ,
	0x3ff , 0xfffe , 0x7fff , 0xc3ff , 0xfe7f ,
	0xfe00 , 0x7ffe , 0x7fe0 , 0x7 , 0xfeff ,
	0xffff , 0xffff , 0x0 , 0x0 , 0xa8 ,
	0x8888 , 

	0x888a , 0xaaaa , 0xaaaa , 0xaa0f , 0xffc3 ,
	0xfff0 , 0x7fff , 0xffff , 0xfe00 , 0x1fff ,
	0xf800 , 0x18 , 0x1818 , 0xff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0x0 , 0x0 , 0x0 , 0x0 ,
	0xff00 , 

	0x3fff , 0xffff , 0xfc3f , 0xffff , 0xfffc ,
	0x3fff , 0xffff , 0xe03f , 0xffff , 0xe000 ,
	0x3fff , 0xffff , 0xfc3f , 0xffff , 0xfffc ,
	0x3fff , 0xffff , 0xfc3f , 0xffff , 0xfffc ,
	0x7ff , 0xffff , 0xfc00 , 0x7ff , 0xfffc ,
	0x3fff , 

	0x81ff , 0xfc3f , 0xfe00 , 0x7ffc , 0x3fc0 ,
	0x3 , 0xfcff , 0xffff , 0xffff , 0x0 ,
	0x0 , 0x51 , 0x4141 , 0x4145 , 0xaaaa ,
	0xaaaa , 0xaa0f , 0xffc3 , 0xfff0 , 0x3fff ,
	0xffff , 0xfc00 , 0x3fff , 0xfc00 , 0x30 ,
	0x180c , 

	0xff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff00 , 0x3fff , 0xffff ,
	0xfc3f , 0xffff , 0xfffc , 0x3fff , 0xffff ,
	0xf83f , 0xffff , 0xf000 , 0x3fff , 0xffff ,
	0xfc3f , 

	0xffff , 0xfffc , 0x3fff , 0xffff , 0xfc3f ,
	0xffff , 0xfffc , 0x1fff , 0xffff , 0xfc00 ,
	0xfff , 0xfffc , 0x3fff , 0x81ff , 0xfc3f ,
	0xfc00 , 0x3ffc , 0x3f80 , 0x1 , 0xfcff ,
	0xffff , 0xffff , 0x0 , 0x0 , 0xa2 ,
	0xa2a2 , 

	0xa2a2 , 0xaaaa , 0xaaaa , 0xaa0f , 0xffc3 ,
	0xfff0 , 0x3fff , 0xffff , 0xfc00 , 0x7fff ,
	0xfe00 , 0x60 , 0x1806 , 0xff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0x0 , 0x0 , 0x0 , 0x0 ,
	0xff00 , 

	0x1fff , 0xffff , 0xf81f , 0xffff , 0xfff8 ,
	0x1fff , 0xffff , 0xf81f , 0xffff , 0xf800 ,
	0x1fff , 0xffff , 0xf81f , 0xffff , 0xfff8 ,
	0x1fff , 0xffff , 0xf81f , 0xffff , 0xfff8 ,
	0x1fff , 0xffff , 0xf800 , 0x1fff , 0xfff8 ,
	0x1fff , 

	0x81ff , 0xf81f , 0xfc00 , 0x3ff8 , 0x1f00 ,
	0x0 , 0xf8ff , 0xffff , 0xffff , 0x0 ,
	0x0 , 0x55 , 0x5555 , 0x5555 , 0xaaaa ,
	0xaaaa , 0xaa07 , 0xff81 , 0xffe0 , 0x1fff ,
	0xffff , 0xf800 , 0xffff , 0xff00 , 0xc0 ,
	0x1803 , 

	0xff , 0xffff , 0xffff , 0xffff , 0xffff ,
	0xffff , 0xffff , 0xffff , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff00 , 0xfff , 0xffff ,
	0xf00f , 0xffff , 0xfff0 , 0xfff , 0xffff ,
	0xf00f , 0xffff , 0xfc00 , 0xfff , 0xffff ,
	0xf00f , 

	0xffff , 0xfff0 , 0xfff , 0xffff , 0xf00f ,
	0xffff , 0xfff0 , 0xfff , 0xffff , 0xf000 ,
	0x3fff , 0xfff0 , 0xfff , 0x81ff , 0xf00f ,
	0xf800 , 0x1ff0 , 0xe00 , 0x0 , 0x70fb ,
	0xfeff , 0x7fdf , 0x0 , 0x0 , 0xaa ,
	0xaaaa , 

	0x2a8a , 0xaaaa , 0xaa2a , 0x8a03 , 0xff00 ,
	0xffc0 , 0xfff , 0xffff , 0xf001 , 0xffff ,
	0xff80 , 0x180 , 0x1801 , 0x80fb , 0xfeff ,
	0x7fdf , 0xfbfe , 0xff7f , 0xdffb , 0xfeff ,
	0x7fdf , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0xfff , 0xffff , 0xf00f , 0xffff , 0xfff0 ,
	0xfff , 0xffff , 0xf00f , 0xffff , 0xfe00 ,
	0xfff , 0xffff , 0xf00f , 0xffff , 0xfff0 ,
	0xfff , 0xffff , 0xf00f , 0xffff , 0xfff0 ,
	0xfff , 0xffff , 0xf000 , 0x7fff , 0xfff0 ,
	0xfff , 

	0xff , 0xf00f , 0xf800 , 0x1ff0 , 0xc00 ,
	0x0 , 0x30fb , 0xfeff , 0x7fdf , 0x0 ,
	0x0 , 0x51 , 0x5455 , 0x5555 , 0xaaaa ,
	0xaa2a , 0x8a01 , 0xfe00 , 0x7f80 , 0xfff ,
	0xffff , 0xf003 , 0xffff , 0xffc0 , 0x300 ,
	0x1800 , 

	0xc0fb , 0xfeff , 0x7fdf , 0xfbfe , 0xff7f ,
	0xdffb , 0xfeff , 0x7fdf , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x7ff , 0xffff ,
	0xe007 , 0xffff , 0xffe0 , 0x7ff , 0xffff ,
	0xe007 , 0xffff , 0xff00 , 0x7ff , 0xffff ,
	0xe007 , 

	0xffff , 0xffe0 , 0x7ff , 0xffff , 0xe007 ,
	0xffff , 0xffe0 , 0x7ff , 0xffff , 0xe000 ,
	0xffff , 0xffe0 , 0x7ff , 0xff , 0xe007 ,
	0xf000 , 0xfe0 , 0x0 , 0x0 , 0xf1 ,
	0xfc7e , 0x3f8f , 0x0 , 0x0 , 0xa0 ,
	0xa82a , 

	0x2a8a , 0xa0a8 , 0x2a2a , 0x8a00 , 0xfc00 ,
	0x3f00 , 0x7ff , 0xffff , 0xe007 , 0xffff ,
	0xffe0 , 0x600 , 0x1800 , 0x60f1 , 0xfc7e ,
	0x3f8f , 0xf1fc , 0x7e3f , 0x8ff1 , 0xfc7e ,
	0x3f8f , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 

	0x3ff , 0xffff , 0xc003 , 0xffff , 0xffc0 ,
	0x3ff , 0xffff , 0xc003 , 0xffff , 0xff80 ,
	0x3ff , 0xffff , 0xc003 , 0xffff , 0xffc0 ,
	0x3ff , 0xffff , 0xc003 , 0xffff , 0xffc0 ,
	0x3ff , 0xffff , 0xc001 , 0xffff , 0xffc0 ,
	0x3ff , 

	0xff , 0xc003 , 0xf000 , 0xfc0 , 0x0 ,
	0x0 , 0xf1 , 0xfc7e , 0x3f8f , 0x0 ,
	0x0 , 0x41 , 0x5454 , 0x1505 , 0xa0a8 ,
	0x2a2a , 0x8a00 , 0x0 , 0x0 , 0x3ff ,
	0xffff , 0xc00f , 0xffe7 , 0xfff0 , 0x0 ,
	0x1800 , 

	0xf1 , 0xfc7e , 0x3f8f , 0xf1fc , 0x7e3f ,
	0x8ff1 , 0xfc7e , 0x3f8f , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff00 , 0x1ff , 0xffff ,
	0x8001 , 0xffff , 0xff80 , 0x1ff , 0xffff ,
	0x8001 , 0xffff , 0xff80 , 0x1ff , 0xffff ,
	0x8001 , 

	0xffff , 0xff80 , 0x1ff , 0xffff , 0x8001 ,
	0xffff , 0xff80 , 0x1ff , 0xffff , 0x8001 ,
	0xffff , 0xff80 , 0x1ff , 0xff , 0x8001 ,
	0xe000 , 0x780 , 0x0 , 0x0 , 0xe0 ,
	0xf83c , 0x1f07 , 0x0 , 0x0 , 0xa0 ,
	0xa828 , 

	0xa02 , 0xa0a8 , 0x280a , 0x200 , 0x0 ,
	0x0 , 0x1ff , 0xffff , 0x801f , 0xffe7 ,
	0xfff8 , 0x0 , 0x1800 , 0xe0 , 0xf83c ,
	0x1f07 , 0xe0f8 , 0x3c1f , 0x7e0 , 0xf83c ,
	0x1f07 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0xff00 , 

	0x7f , 0xfffe , 0x0 , 0x7fff , 0xfe00 ,
	0x7f , 0xfffe , 0x0 , 0x7fff , 0xfe00 ,
	0x7f , 0xfffe , 0x0 , 0x7fff , 0xfe00 ,
	0x7f , 0xfffe , 0x0 , 0x7fff , 0xfe00 ,
	0x7f , 0xfffe , 0x0 , 0x7fff , 0xfe00 ,
	0x7e , 

	0x7e , 0x0 , 0x6000 , 0x600 , 0x0 ,
	0x0 , 0xe0 , 0xf83c , 0x1f07 , 0x0 ,
	0x0 , 0x40 , 0x5014 , 0x1505 , 0xa0a8 ,
	0x280a , 0x200 , 0x0 , 0x0 , 0x7f ,
	0xfffe , 0x3f , 0xff81 , 0xfffc , 0x0 ,
	0x1800 , 

	0xe0 , 0xf83c , 0x1f07 , 0xe0f8 , 0x3c1f ,
	0x7e0 , 0xf83c , 0x1f07 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff00 , 0x3f , 0xfffc ,
	0x0 , 0x3fff , 0xfc00 , 0x3f , 0xfffc ,
	0x0 , 0x3fff , 0xfc00 , 0x3f , 0xfffc ,
	0x0 , 

	0x3fff , 0xfc00 , 0x3f , 0xfffc , 0x0 ,
	0x3fff , 0xfc00 , 0x3f , 0xfffc , 0x0 ,
	0x3fff , 0xfc00 , 0x3e , 0x7c , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0xc0 ,
	0x7018 , 0xe03 , 0x0 , 0x0 , 0x80 ,
	0x2008 , 

	0xa02 , 0x8020 , 0x80a , 0x200 , 0x0 ,
	0x0 , 0x3f , 0xfffc , 0x7f , 0xff00 ,
	0xfffe , 0x0 , 0x1800 , 0xc0 , 0x7018 ,
	0xe03 , 0xc070 , 0x180e , 0x3c0 , 0x7018 ,
	0xe03 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0xff00 , 

	0xf , 0xfff0 , 0x0 , 0xfff , 0xf000 ,
	0xf , 0xfff0 , 0x0 , 0xfff , 0xf000 ,
	0xf , 0xfff0 , 0x0 , 0xfff , 0xf000 ,
	0xf , 0xfff0 , 0x0 , 0xfff , 0xf000 ,
	0xf , 0xfff0 , 0x0 , 0xfff , 0xf000 ,
	0xe , 

	0x70 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0xc0 , 0x7018 , 0xe03 , 0x0 ,
	0x0 , 0x40 , 0x5010 , 0x401 , 0x8020 ,
	0x80a , 0x200 , 0x0 , 0x0 , 0xf ,
	0xfff0 , 0xff , 0xfc00 , 0x3fff , 0x0 ,
	0x1800 , 

	0xc0 , 0x7018 , 0xe03 , 0xc070 , 0x180e ,
	0x3c0 , 0x7018 , 0xe03 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff00 , 0x0 , 0xff00 ,
	0x0 , 0xff , 0x0 , 0x0 , 0xff00 ,
	0x0 , 0xff , 0x0 , 0x0 , 0xff00 ,
	0x0 , 

	0xff , 0x0 , 0x0 , 0xff00 , 0x0 ,
	0xff , 0x0 , 0x0 , 0xff00 , 0x0 ,
	0xff , 0x0 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0x0 , 0x0 , 0x80 ,
	0x2000 , 0x401 , 0x0 , 0x0 , 0x80 ,
	0x2000 , 

	0x0 , 0x8020 , 0x0 , 0x0 , 0x0 ,
	0x0 , 0x0 , 0xff00 , 0x3f , 0xc000 ,
	0x3fc , 0x0 , 0x1800 , 0x80 , 0x2000 ,
	0x401 , 0x8020 , 0x4 , 0x180 , 0x2000 ,
	0x401 , 0x0 , 0x0 , 0x0 , 0x0 ,
	0xff00

};

