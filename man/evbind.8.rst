========
 evbind
========

listen to input devices and execute arbitrary commands
------------------------------------------------------

:Date: 2012-10-30
:Version: 0.1
:Manual section: 8

SYNOPSIS
========

evbind [OPTION]...

DESCRIPTION
===========

evbind monitors input event devices and executes arbitrary programs
bound to individual events or event combinations. By default, evbind
detaches itself from the controlling terminal runs in the background as
a daemon process. This can disabled with **--no-daemon** flag.

OPTIONS
=======

--no-daemon

       Do not run as a daemon. Information and error messages are always
       printed to syslog. If evbind is not running as a daemon, messages
       are printed also to stderr.

-h, --help

       Display help and exit.

-V, --version

       Output version information and exit.

FILES
=====

`/etc/evbind/events`
  This directory contains all event binding configurations.

AUTHOR
======

Tuomas Jorma Juhani Räsänen <tuomasjjrasanen@tjjr.fi>

COPYRIGHT
=========

Copyright © 2012 Tuomas Jorma Juhani Räsänen. License GPLv3+: GNU GPL
version 3 or later <http://gnu.org/licenses/gpl.html>. This is free
software: you are free to change and redistribute it. There is NO
WARRANTY, to the extent permitted by law.

SEE ALSO
========

sshd(8)

Homepage: http://tjjr.fi/sw/evbind/
