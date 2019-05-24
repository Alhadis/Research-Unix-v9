/*	@(#)fort.h	1.1	86/02/03	SMI	*/

/*	machine dependent file  */

label( n ){
	print_label(n);
	}

tlabel(){
	lccopy( 2 );
	print_str( ":\n" );
	}
