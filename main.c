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

#include <libudev.h>

#include "err.h"

extern char *program_invocation_name;
extern char *program_invocation_short_name;

static bool no_daemon = false;

static void evb_main_help_and_exit(void)
{
        fprintf(stderr, "Try `%s --help' for more information.\n",
                program_invocation_name);
        exit(EXIT_FAILURE);
}

static void evb_main_parse_args(int argc, char **argv)
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
                        evb_main_help_and_exit();
                default:
                        errx(EXIT_FAILURE, "argument parsing failed");
                }
        }

        if (optind != argc) {
                fprintf(stderr, "%s: wrong number of arguments\n",
                        program_invocation_name);
                evb_main_help_and_exit();
        }
}

int evb_main_daemonize(struct evb_err *const err)
{
	int fd_devnull;

	switch (fork()) {
	case 0:
		break;
	case -1:
		evb_err_set(err, EVB_ERR_NUM_SYS, "failed to fork: %s",
			    strerror(errno));
		return -1;
	default:
		_exit(0);
	}

	setsid();

	switch (fork()) {
	case 0:
		break;
	case -1:
		evb_err_set(err, EVB_ERR_NUM_SYS, "failed to double-fork: %s",
			    strerror(errno));
		return -1;
	default:
		_exit(0);
	}

	if (chdir("/") == -1) {
		evb_err_set(err, EVB_ERR_NUM_SYS,
			    "failed to chdir to /: %s",
			    strerror(errno));
		return -1;
	}

	fd_devnull = open("/dev/null", O_RDWR);
	if (fd_devnull == -1) {
		evb_err_set(err, EVB_ERR_NUM_SYS,
			    "failed to open /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	if (dup2(fd_devnull, 0) == -1) {
		evb_err_set(err, EVB_ERR_NUM_SYS,
			    "failed to redirect stdin to /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	if (dup2(fd_devnull, 1) == -1) {
		evb_err_set(err, EVB_ERR_NUM_SYS,
			    "failed to redirect stdout to /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	if (dup2(fd_devnull, 2) == -1) {
		evb_err_set(err, EVB_ERR_NUM_SYS,
			    "failed to redirect stderr to /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	umask(0);

	return 0;
}

static char **evb_main_find_devnodes(size_t *const len, struct evb_err *const err)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *list_entry;
	struct udev_list_entry *list_entries;
	char **devnodev = NULL;
	size_t devnodec = 0;

	udev = udev_new();
	if (!udev) {
		evb_err_set(err, EVB_ERR_NUM_UDEV, "%s", "udev_new returned NULL");
		goto out;
	}

	enumerate = udev_enumerate_new(udev);
	if (!enumerate) {
		evb_err_set(err, EVB_ERR_NUM_UDEV, "%s", "udev_enumerate_new returned NULL");
		goto out;
	}

	if (udev_enumerate_add_match_subsystem(enumerate, "input")) {
		evb_err_set(err, EVB_ERR_NUM_UDEV, "%s", "udev_enumerate_add_match_subsystem failed");
		goto out;
	}

	if (udev_enumerate_scan_devices(enumerate)) {
		evb_err_set(err, EVB_ERR_NUM_UDEV, "%s", "udev_enumerate_scan_devices failed");
		goto out;
	}

	list_entries = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(list_entry, list_entries) {
		const char *path;
		struct udev_device *dev;
		char **devnodev_new;
		const char *devnode;

		path = udev_list_entry_get_name(list_entry);

		dev = udev_device_new_from_syspath(udev, path);
		if (!dev) {
			evb_err_set(err, EVB_ERR_NUM_UDEV, "%s",
				    "failed to create udev device");
			goto out;
		}

		if (strncmp(udev_device_get_sysname(dev), "event", 5))
			goto continue_loop;

		devnode = udev_device_get_devnode(dev);

		devnodev_new = realloc(devnodev, sizeof(char *) * (devnodec + 1));
		if (!devnodev_new) {
			evb_err_set(err, EVB_ERR_NUM_SYS, "%s",
				    "failed to allocate memory for devnodes");
			udev_device_unref(dev);
			goto out;
		}
		devnodev = devnodev_new;

		devnodev[devnodec] = malloc(sizeof(char) *  (strlen(devnode) + 1));
		if (!devnodev[devnodec]) {
			evb_err_set(err, EVB_ERR_NUM_SYS, "%s",
				    "failed to allocate memory for devnode");
			udev_device_unref(dev);
			goto out;
		}
		strcpy(devnodev[devnodec], devnode);
		++devnodec;

	continue_loop:
		udev_device_unref(dev);
	}

	*len = devnodec;
out:
	if (devnodev && evb_err_num(err)) {
		size_t i;
		for (i = 0; i < devnodec; ++i) {
			free(devnodev[i]);
		}
		free(devnodev);
		devnodev = NULL;
	}

	if (enumerate)
		udev_enumerate_unref(enumerate);

	if (udev)
		udev_unref(udev);

	return devnodev;
}

int main(int argc, char **argv)
{
	int exitval = EXIT_FAILURE;
	struct evb_err *err;
	char **devnodev;
	size_t devnodec;
	size_t i;

	evb_main_parse_args(argc, argv);

        openlog(program_invocation_short_name, LOG_ODELAY | LOG_PERROR,
		LOG_DAEMON);

	err = evb_err_new();
	if (!err) {
		syslog(LOG_ERR, "evb_err_new() failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	if (!no_daemon && evb_main_daemonize(err) == -1)
		goto out;

	devnodev = evb_main_find_devnodes(&devnodec, err);
	if (!devnodev)
		goto out;

	for (i = 0; i < devnodec; ++i) {
		syslog(LOG_INFO, "found: %s", devnodev[i]);
		free(devnodev[i]);
	}

	free(devnodev);

	exitval = EXIT_SUCCESS;
out:
	if (evb_err_num(err))
		syslog(LOG_ERR, "%s", evb_err_str(err));

	evb_err_free(err);

        syslog(LOG_INFO, "%s", "terminated");

	return exitval;
}
