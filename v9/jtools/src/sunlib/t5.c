/*
	hierarchical menu test
dfn() [f1] gets called when moving to a pop-aside menu
		also gets called when moving off right edge of any
		non-heirarchical menu!--f3 gets called immediately after!
bfn() [f2] gets called when moving back to a lower menu
		also gets called after f3 -- once per pop-aside menu
hfn() [f3] gets called when item is selected
*/

#include	"menu.h"
#include	"jerq.h"

#define DEFHEIGHT 14
#define CLEARLN 	{	rectf(&display,Rpt(Drect.org,		\
			Pt(Drect.cor.x,Drect.org.y+DEFHEIGHT)),F_CLR);}
#define PRINT(S) 	{printf("%s\n",S);}
#define oPRINT(S) 	{	CLEARLN;				\
			string(&defont,(S),&display,Drect.org,F_STORE);}				
void quit(), f1(), f2(), f3();

static NMenu m1, m2, m3, m4;
NMitem item1[] = {
		"gods",	"help me please!",		0,f1,f2,f3,0,
		"men",	"help me please!",		0,f1,f2,f3,1,
		"art",	"help me please!",		0,f1,f2,f3,2,
		"snarf","help me please!",		0,f1,f2,f3,3,
		0
};
NMitem item2[] = {
		"add all","help me please!",		0,f1,f2,f3,4,
		"free all","help me please!",		&m2,f1,f2,f3,5,
		"move","help me please!",		0,f1,f2,f3,6,
		"top","help me please!",		0,f1,f2,f3,7,
		"bottom","help me please!",		0,f1,f2,f3,8,
		"mark/unmark","help me please!",	&m1,f1,f2,f3,9,
		"free","help me please!",		0,f1,f2,f3,10,
		"add all","help me please!",		0,f1,f2,f3,4,
		"free all","help me please!",		&m2,f1,f2,f3,5,
		"move","help me please!",		0,f1,f2,f3,6,
		"top","help me please!",		0,f1,f2,f3,7,
		"bottom","help me please!",		0,f1,f2,f3,8,
		"mark/unmark","help me please!",	&m1,f1,f2,f3,9,
		"free","help me please!",		0,f1,f2,f3,10,
		"add all","help me please!",		0,f1,f2,f3,4,
		"free all","help me please!",		&m2,f1,f2,f3,5,
		"move","help me please!",		0,f1,f2,f3,6,
		"top","help me please!",		0,f1,f2,f3,7,
		"bottom","help me please!",		0,f1,f2,f3,8,
		"mark/unmark","help me please!",	&m1,f1,f2,f3,9,
		"free","help me please!",		0,f1,f2,f3,10,
		0
};
NMitem item3[] ={
		"save","help me please!",		0,f1,f2,f3,11,
		"read","help me please!",		&m4,f1,f2,f3,12,
		"quit","help me please!",		0,f1,f2,f3,13,
		0
};
#ifdef old
NMitem item1[] = {
		"gods",	"help me please!",		0,0,0,0,0,
		"men",	"help me please!",		0,0,0,0,1,
		"art",	"help me please!",		0,0,0,0,2,
		"philosophy","help me please!",	0,0,0,0,3,
		0
};
NMitem item2[] = {
		"add all","help me please!",		0,0,0,0,4,
		"free all","help me please!",		&m2,0,0,0,5,
		"move","help me please!",		0,0,0,0,6,
		"top","help me please!",		0,0,0,0,7,
		"bottom","help me please!",		0,0,0,0,8,
		"mark/unmark","help me please!",	&m1,0,0,0,9,
		"free","help me please!",		0,0,0,0,10,
		0
};
NMitem item3[] ={
		"save","help me please!",		0,0,0,0,11,
		"read","help me please!",		&m4,0,0,0,12,
		"quit","help me please!",		0,0,0,0,13,
		0
};
#endif
int a = 0;
int b = 0;
int c = 0;
char str[50];
char *s = str;

NMitem*
gen_func(n)
int n;
{
	static NMitem men_item[4];

	men_item[0].text = "zero";
	men_item[1].text = "one";
	men_item[2].text = "two";
	men_item[3].text = 0;
	men_item[0].data = 0;
	men_item[1].data = 1;
	men_item[2].data = 2;
	men_item[3].data = 0;

	if(n > 2)
		return &men_item[3];
	return &men_item[n];
}

main (argc, argv)
int argc;
char **argv;
{
	NMitem *gen_func();
	NMitem *temp;

	request(MOUSE);
	initdisplay(argc, argv);
	m1.item = item1;
	m2.item = item2;
	m3.item = item3;
	m4.item = 0;
	m4.generator = gen_func;
	for(;;){
		nap(1);
		if(button1() && (temp = hmenuhit(&m1,1))){
			switch(temp->data){
			case 0:
				PRINT("m1, case 0");
				break;
			case 1:
				PRINT("m1, case 1");
				break;
			case 2:
				PRINT("m1, case 2");
				break;
			case 3:
				PRINT("m1, case 3");
				break;
			}
		/*	while(button1())
				nap(1);*/
		}
		else if(button2() && (temp = hmenuhit(&m2,2))){
			switch(temp->data){
			case 0:
				PRINT("m1, case 0");
				break;
			case 1:
				PRINT("m1, case 1");
				break;
			case 2:
				PRINT("m1, case 2");
				break;
			case 3:
				PRINT("m1, case 3");
				break;
			case 4:
				PRINT("m2, case 0");
				break;
			case 5:
				PRINT("m2, case 1");
				break;
			case 6:
				PRINT("m2, case 2");
				break;
			case 7:
				PRINT("m2, case 3");
				break;
			case 8:
				PRINT("m2, case 4");
				break;
			case 9:
				PRINT("m2, case 5");
				break;
			case 10:
				PRINT("m2, case 6");
				break;
			}
		/*	while(button2())
				nap(2);*/
		}
		else if(button3() && (temp = hmenuhit(&m3,3))){
			switch(temp->data){
			case 0:
				PRINT("m4, case 0");
				break;
			case 1:
				PRINT("m4, case 1");
				break;
			case 2:
				PRINT("m4, case 2");
				break;
			case 11:
				PRINT("m3, case 0");
				break;
			case 12:
				PRINT("m3, case 1");
				break;
			case 13:
				quit();
				break;
			}
		/*	while(button3())
				nap(1);*/
		}
	}
}

void
quit()
{
	PRINT("quitting soon");
	exit();
}

void
f1()
{
	sprintf(s,"f1(): a = %d",a);
	PRINT(s);
/*	nap(2);*/
	a++;
}

void
f2()
{
	sprintf(s,"f2(): b = %d",b);
	PRINT(s);
/*	nap(2);*/
	b++;
}

void
f3()
{
	sprintf(s,"f3(): c = %d",c);
	PRINT(s);
/*	nap(2);*/
	c++;
}
