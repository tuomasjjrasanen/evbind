/*
  Copyright (C) 2012 Tuomas Jorma Juhani Räsänen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern char *program_invocation_name;
extern char *program_invocation_short_name;

static bool no_daemon = false;

static void evb_help_and_exit(void)
{
        fprintf(stderr, "Try `%s --help' for more information.\n",
                program_invocation_name);
        exit(EXIT_FAILURE);
}

static void evb_parse_args(int argc, char **argv)
{
        const struct option options[] = {
                {"no-daemon", no_argument, NULL, 'n'},
                {"version", no_argument, NULL, 'V'},
                {"help", no_argument, NULL, 'h'},
                {0, 0, 0, 0}
        };

        while (1) {
                int option;

                option = getopt_long(argc, argv, "Vh", options, NULL);

                if (option == -1)
                        break;

                switch (option) {
                case 'n':
                        no_daemon = true;
                        break;
                case 'V':
                        printf("evbind 0.1\n"
                               "Copyright (C) 2012 Tuomas Jorma Juhani Räsänen\n"
                               "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
                               "This is free software: you are free to change and redistribute it.\n"
                               "There is NO WARRANTY, to the extent permitted by law.\n"
			       "\n"
			       "Written by Tuomas Jorma Juhani Räsänen.\n");
                        exit(EXIT_SUCCESS);
                case 'h':
                        printf("Usage: %s [OPTION]...\n"
                               "Evbind monitors input event devices and executes arbitrary\n"
			       "programs bound to individual events or event combinations.\n"
			       "\n"
			       "Bindings are defined in /etc/evbind/events directory.\n"
                               "\n"
                               "Options:\n"
                               "     --no-daemon            do not run as a daemon process\n"
                               " -h, --help                 display this help and exit\n"
                               " -V, --version              output version information and exit\n"
                               "\n"
                               "Processing information and error messages are always printed into syslog.\n"
                               "If evbind is not running as a daemon, logs are also printed into stderr.\n"
                               "\n"
                               "Homepage: http://tjjr.fi/sw/evbind/\n"
			       "\n", program_invocation_name);
                        exit(EXIT_SUCCESS);
                case '?':
                        evb_help_and_exit();
                default:
                        errx(EXIT_FAILURE, "argument parsing failed");
                }
        }

        if (optind != argc) {
                fprintf(stderr, "%s: wrong number of arguments\n",
                        program_invocation_name);
                evb_help_and_exit();
        }
}

int main(int argc, char **argv)
{
	evb_parse_args(argc, argv);

        openlog(program_invocation_short_name, LOG_ODELAY | LOG_PERROR,
		LOG_DAEMON);

	if (!no_daemon) {
		int fd_devnull;

		switch (fork()) {
		case 0:
			break;
		case -1:
			syslog(LOG_ERR, "failed to fork: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		default:
			_exit(0);
		}

		setsid();

		switch (fork()) {
		case 0:
			break;
		case -1:
			syslog(LOG_ERR, "failed to double-fork: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		default:
			_exit(0);
		}

		if (chdir("/") == -1) {
			syslog(LOG_ERR, "failed to change working directory to /: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		}

		fd_devnull = open("/dev/null", O_RDWR);
		if (fd_devnull == -1) {
			syslog(LOG_ERR, "failed to open /dev/null: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		}

		if (dup2(fd_devnull, 0) == -1) {
			syslog(LOG_ERR, "failed to redirect stdin to /dev/null: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		}

		if (dup2(fd_devnull, 1) == -1) {
			syslog(LOG_ERR, "failed to redirect stdout to /dev/null: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		}

		if (dup2(fd_devnull, 2) == -1) {
			syslog(LOG_ERR, "failed to redirect stderr to /dev/null: %s",
			       strerror(errno));
			return EXIT_FAILURE;
		}

		umask(0);
	}

        syslog(LOG_INFO, "terminated");

	return EXIT_SUCCESS;
}
