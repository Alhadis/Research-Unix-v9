#include    "sun.h"
#include    "opaque.h"

Bool	    	screenSaved = FALSE;
int	    	lastEventTime = 0;
extern void	SaveScreens();
extern long EnabledDevices, LastSelectMask[];
extern int sunKbdfd;
extern int sunMousefd;

/*-
 *-----------------------------------------------------------------------
 * TimeSinceLastInputEvent --
 *	Function used for screensaver purposes by the os module.
 *
 * Results:
 *	The time in milliseconds since there last was any
 *	input.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
TimeSinceLastInputEvent()
{
	struct timeb now;

	ftime(&now);
	if (!lastEventTime)
		lastEventTime = now.time * 1000 + now.millitm;
	return now.time * 1000 + now.millitm - lastEventTime;
}

/*-
 *-----------------------------------------------------------------------
 * ProcessInputEvents --
 *	Retrieve all waiting input events and pass them to DIX in their
 *	correct chronological order. Only reads from the system pointer
 *	and keyboard.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Events are passed to the DIX layer.
 *
 *-----------------------------------------------------------------------
 */
void
ProcessInputEvents ()
{
	DevicePtr	pPointer;
	DevicePtr	pKeyboard;

	pPointer = LookupPointerDevice();
	pKeyboard = LookupKeyboardDevice();

	if (LastSelectMask[0] & EnabledDevices) {
		SetTimeSinceLastInputEvent();
		if (LastSelectMask[0] & (1 << sunMousefd)) {
			sunMouseEvent(pPointer);
			LastSelectMask[0] &= ~(1 << sunMousefd);
		}
		if (LastSelectMask[0] & (1 << sunKbdfd)) {
			sunKbdEvent(pKeyboard);
			LastSelectMask[0] &= ~(1 << sunKbdfd);
		}
		if (screenSaved)
			SaveScreens(SCREEN_SAVER_FORCER, ScreenSaverReset);
	}
	sunRestoreCursor();
}


/*-
 *-----------------------------------------------------------------------
 * SetTimeSinceLastInputEvent --
 *	Set the lastEventTime to now.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	lastEventTime is altered.
 *
 *-----------------------------------------------------------------------
 */
void
SetTimeSinceLastInputEvent()
{
	struct timeb	now;

	ftime(&now);
	lastEventTime = now.time * 1000 + now.millitm;
}
