IdleTaskTerminatorLite
======================

We use an old custom `shutdown.exe` (BeyondLogic Shutdown) to provide a timed screen lock feature, where a user is notified their screen will lock
in a period of time and can cancel the locking of the workstation.

Clicking the Cancel button within the time limit, however, seems unnecessary and requires precisely clicking the Cancel button when
the user is under time pressure! This is not a good user experience. A simple change to the idle state of the machine (any keypress or mouse movement) should cancel
the timed locking of the workstation.

This lightweight background application detects user activity and forcibly kills the `beyondlogic.shutdown.exe` process, effectively cancelling the
locking of the workstation without requiring the user to actually click Cancel.

This is currently rather 'opinionated' in that it specifically checks for hard-coded named processes running. It Works for Us(tm), but you may
need to modify it for your environment. ;)

C -- Win32 API.
