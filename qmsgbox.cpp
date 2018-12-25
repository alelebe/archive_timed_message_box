
#include <windows.h>

#define MAXLEN 1024
#define GMSGBOX_TIMER 1231967

static DWORD FAR PASCAL gmsgboxhookproc (int, WORD, DWORD);
static long FAR PASCAL gmsgboxwndproc (HWND, UINT, UINT, LONG);

static HHOOK gmsgboxhook = NULL;
static UINT gmsgboxtimer = 0;
static WNDPROC gmsgboxoldwndproc = NULL;
static UINT gmsgboxtimeout = 0;
static char *gmsgboxcaption = NULL;

int gmsgbox (char *boxmsg, char *title, int flags, int timeout)
{
    int result;

    gmsgboxcaption = title;     /* Make static copy of pointer to caption. */
    if (timeout > 0) {                  /* If a timeout is specified... */
        gmsgboxtimeout = timeout;       /* Make global copy of "timeout". */
        gmsgboxhook = SetWindowsHookEx (WH_CALLWNDPROCRET,
        (HOOKPROC) gmsgboxhookproc, NULL, GetCurrentThreadId ());
    }

    result = MessageBox (NULL, boxmsg, title, flags | MB_SETFOREGROUND);

    gmsgboxcaption = NULL;
    if (gmsgboxhook != NULL) {
        (void) UnhookWindowsHookEx (gmsgboxhook);
        gmsgboxhook = NULL;
    }

    return (result);
}


/* Message box hook - if a timeout is necessary, this routine
    senses the setup of the message box and sets a timer. */

static DWORD FAR PASCAL gmsgboxhookproc (int code, WORD wparam, DWORD lparam)
{
    BOOL doit;                  /* Continue processing. */
    LPCWPRETSTRUCT lpmsg;
    char windowtext[MAXLEN];    /* Windows caption text */

    doit = ((code >= 0) && (gmsgboxcaption != NULL));
    if (doit) {
        lpmsg = (LPCWPRETSTRUCT) lparam;
        doit = (lpmsg->message == WM_INITDIALOG);
    }
    if (doit) {
        GetWindowText (lpmsg->hwnd, windowtext, sizeof (windowtext));
        doit = (strcmp (windowtext, gmsgboxcaption) == 0);
    }
    if (doit) {         /* Subclass the message box and set a timer. */
        gmsgboxoldwndproc = (WNDPROC) GetWindowLong (lpmsg->hwnd,
GWL_WNDPROC);
        SetWindowLong (lpmsg->hwnd, GWL_WNDPROC,
        (long) MakeProcInstance ((FARPROC) gmsgboxwndproc, ginstance));
        gmsgboxtimer = SetTimer (lpmsg->hwnd, GMSGBOX_TIMER,
        gmsgboxtimeout * 1000, NULL);
    }
    return (CallNextHookEx (gmsgboxhook, code, wparam, lparam));
}


/* Subclassed message box window procedure */

static long FAR PASCAL gmsgboxwndproc (
HWND wh,
UINT message,
UINT wparam,
LONG lparam)
{
    LRESULT rc;
    DWORD dwork;

/* Timer tick - if the timer has run out, close the message box. */

    if ((message == WM_TIMER)
    && (gmsgboxtimer > 0)
    && (wparam == gmsgboxtimer)) {
        KillTimer (wh, gmsgboxtimer);
        dwork = SendMessage (wh, DM_GETDEFID, 0, 0);
        EndDialog (wh, LOWORD (dwork));
        return (0);
    }

/* If the message box is closing, clean up our hacks. */

    if (message == WM_NCDESTROY) {

        if (gmsgboxoldwndproc != NULL) {

        /* Call the original window procedure first,
            so it's done before we remove the subclassing. */

            rc = CallWindowProc (gmsgboxoldwndproc,
            wh, message, wparam, lparam);

        /* Now remove the subclassed procedure that the hook installed. */

            SetWindowLong (wh, GWL_WNDPROC, (long) gmsgboxoldwndproc);
            gmsgboxoldwndproc = NULL;

            return (rc);
        }
    }

/* Default window procedure */

    return (CallWindowProc (gmsgboxoldwndproc, wh, message, wparam, lparam));
}

void main()
{
	gmsgbox ("Hello, World!!!", "qmsgbox", MB_OK, 3);
}
