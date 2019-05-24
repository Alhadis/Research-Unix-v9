struct Jrcvbuf {
	unsigned char *buf;
	unsigned char *in;
	unsigned char *out;
	int cnt;
	int size;
	int blocked;
};
extern struct Jrcvbuf Jrcvbuf;
