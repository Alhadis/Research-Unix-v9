#include "../h/types.h"
#include "../h/acct.h"
#include "../h/buf.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/dkbad.h"
#include "../h/ethernet.h"
#include "../h/fblk.h"
#include "../h/file.h"
#include "../h/filio.h"
#include "../h/filsys.h"
#include "../h/ino.h"
#include "../h/inode.h"
#include "../h/lnode.h"
#include "../h/map.h"
#include "../h/mount.h"
#include "../h/msgbuf.h"
#include "../h/netc.h"
#include "../h/ttyio.h"
#include "../h/nttyio.h"
#include "../h/ttyld.h"
#include "../h/nttyld.h"
#include "../h/proc.h"
#include "../h/retlim.h"
#include "../h/share.h"
#include "../h/stat.h"
#include "../h/stream.h"
#include "../h/systm.h"
#include "../h/text.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/utsname.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#include "../h/wait.h"
#include "../machine/buserr.h"
#include "../machine/clock.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/eccreg.h"
#include "../machine/eeprom.h"
#include "../machine/frame.h"
#include "../machine/idprom.h"
#include "../machine/mbvar.h"
#include "../machine/memerr.h"
#include "../machine/pte.h"
#include "../machine/scb.h"
#include "../machine/sunromvec.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../sundev/iem.h"
#include "../sundev/ieob.h"
#include "../sundev/iereg.h"
#include "../sundev/kbdvar.h"
#include "../sundev/sireg.h"
#include "../sundev/screg.h"
#include "../sundev/scsi.h"
#include "../sundev/zscom.h"
#include "../sundev/zsreg.h"
#define ZSIBUFSZ	256
struct	zsaline {
/* from v9 dz.c */
	short	state;
	short	flags;
	struct	block	*oblock;
	struct	queue	*rdq;
	char	speed;
/* from SUN zsasync.c */
	short	za_needsoft;		/* need for software interrupt */
	short	za_break;		/* break occurred */
	short	za_overrun;		/* overrun (either hw or sw) */
	short	za_ext;			/* modem status change */
	short	za_work;		/* work to do */
	u_char	za_rr0;			/* for break detection */
	u_char	za_ibuf[ZSIBUFSZ];	/* circular input buffer */
	short	za_iptr;		/* producing ptr for input */
	short	za_sptr;		/* consuming ptr for input */
/* additions */
	dev_t	za_dev;
	struct	zscom *za_addr;
};
#include "../h/inet/in.h"
#include "../h/inet/ip.h"
#include "../h/inet/ip_var.h"
#include "../h/inet/tcp.h"
#include "../h/inet/socket.h"
#include "../h/inet/tcp_user.h"
#include "../h/inet/tcp_timer.h"
#include "../h/inet/tcp_var.h"
#include "../h/inet/tcpip.h"
#include "../h/inet/udp.h"
#include "../h/inet/udp_user.h"
#include "../h/inet/udp_var.h"
