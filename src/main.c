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
#define _GNU_SOURCE
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libudev.h>

#include "err.h"

extern char *program_invocation_name;
extern char *program_invocation_short_name;

static bool main_no_daemon = false;

static struct evb_err *main_err = NULL;
static struct udev *main_udev = NULL;
static struct udev_monitor *main_udev_mon = NULL;
static int main_udev_mon_fd = -1;
static fd_set main_evdev_fds;
static int main_evdev_max_fd = -1;
volatile sig_atomic_t main_is_stopped;

static int max(int a, int b)
{
	return a > b ? a : b;
}

static void main_free()
{
	for (int fd = 0; fd <= main_evdev_max_fd; ++fd) {
		if (FD_ISSET(fd, &main_evdev_fds)) {
			close(fd);
		}
	}
	FD_ZERO(&main_evdev_fds);
	main_evdev_max_fd = -1;

	main_udev_mon_fd = -1;

	if (main_udev_mon) {
		udev_monitor_unref(main_udev_mon);
		main_udev_mon = NULL;
	}

	if (main_udev) {
		udev_unref(main_udev);
		main_udev = NULL;
	}

	if (main_err) {
		evb_err_free(main_err);
		main_udev = NULL;
	}
}

static int main_init()
{
        openlog(program_invocation_short_name, LOG_ODELAY | LOG_PERROR,
		LOG_DAEMON);

	main_err = evb_err_new();
	if (!main_err) {
		syslog(LOG_ERR, "evb_err_new() failed: %s",
		       strerror(errno));
		goto err;
	}

	main_udev = udev_new();
	if (!main_udev) {
		syslog(LOG_ERR, "udev_new() failed");
		goto err;
	}

	main_udev_mon = udev_monitor_new_from_netlink(main_udev, "udev");
	if (!main_udev_mon) {
		syslog(LOG_ERR, "%s", "udev_monitor_new_from_netlink() failed");
		goto err;
	}

	if (udev_monitor_filter_add_match_subsystem_devtype(main_udev_mon,
							    "input", NULL)) {
		syslog(LOG_ERR, "%s", "failed to set udev monitor filter");
		goto err;
	}

	if (udev_monitor_enable_receiving(main_udev_mon)) {
		syslog(LOG_ERR, "%s", "failed to start udev monitor");
		goto err;
	}

	main_udev_mon_fd = udev_monitor_get_fd(main_udev_mon);
	if (main_udev_mon_fd < 0) {
		syslog(LOG_ERR, "%s",
		       "got illegal udev monitor file descriptor");
		goto err;
	}

	FD_ZERO(&main_evdev_fds);

	return 0;
err:
	main_free();
	return 1;
}

static void main_help_and_exit()
{
        fprintf(stderr, "Try `%s --help' for more information.\n",
                program_invocation_name);
        exit(EXIT_FAILURE);
}

static void main_parse_args(int argc, char **argv)
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
                        main_no_daemon = true;
                        break;
                case 'V':
                        printf("\
evbind 0.1\n\
Copyright (C) 2012 Tuomas Jorma Juhani Räsänen\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Tuomas Jorma Juhani Räsänen.\n\
");
                        exit(EXIT_SUCCESS);
                case 'h':
                        printf("\
Usage: %s [OPTION]...\n\
evbind monitors input event devices and executes arbitrary\n\
programs bound to individual events or event combinations.\n\
\n\
Options:\n\
     --no-daemon            do not run as a daemon\n\
 -h, --help                 display this help and exit\n\
 -V, --version              output version information and exit\n\
", program_invocation_name);
                        exit(EXIT_SUCCESS);
                case '?':
                        main_help_and_exit();
                default:
			fprintf(stderr, "%s: argument parsing failed",
				program_invocation_name);
			exit(EXIT_FAILURE);
                }
        }

        if (optind != argc) {
                fprintf(stderr, "%s: wrong number of arguments\n",
                        program_invocation_name);
                main_help_and_exit();
        }
}

int main_daemonize()
{
	int fd_devnull;

	switch (fork()) {
	case 0:
		break;
	case -1:
		evb_err_set(main_err, EVB_ERR_NUM_SYS, "failed to fork: %s",
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
		evb_err_set(main_err, EVB_ERR_NUM_SYS,
			    "failed to double-fork: %s",
			    strerror(errno));
		return -1;
	default:
		_exit(0);
	}

	if (chdir("/") == -1) {
		evb_err_set(main_err, EVB_ERR_NUM_SYS,
			    "failed to chdir to /: %s",
			    strerror(errno));
		return -1;
	}

	fd_devnull = open("/dev/null", O_RDWR);
	if (fd_devnull == -1) {
		evb_err_set(main_err, EVB_ERR_NUM_SYS,
			    "failed to open /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	if (dup2(fd_devnull, 0) == -1) {
		evb_err_set(main_err, EVB_ERR_NUM_SYS,
			    "failed to redirect stdin to /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	if (dup2(fd_devnull, 1) == -1) {
		evb_err_set(main_err, EVB_ERR_NUM_SYS,
			    "failed to redirect stdout to /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	if (dup2(fd_devnull, 2) == -1) {
		evb_err_set(main_err, EVB_ERR_NUM_SYS,
			    "failed to redirect stderr to /dev/null: %s",
			    strerror(errno));
		return -1;
	}

	umask(0);

	return 0;
}

static char **main_get_evdevs(size_t *const len)
{
	struct udev_enumerate *enumerate;
	struct udev_list_entry *list_entry;
	struct udev_list_entry *list_entries;
	char **devnodev = NULL;
	size_t devnodec = 0;

	enumerate = udev_enumerate_new(main_udev);
	if (!enumerate) {
		evb_err_set(main_err, EVB_ERR_NUM_UDEV, "%s",
			    "udev_enumerate_new returned NULL");
		goto out;
	}

	if (udev_enumerate_add_match_subsystem(enumerate, "input")) {
		evb_err_set(main_err, EVB_ERR_NUM_UDEV, "%s",
			    "udev_enumerate_add_match_subsystem failed");
		goto out;
	}

	if (udev_enumerate_scan_devices(enumerate)) {
		evb_err_set(main_err, EVB_ERR_NUM_UDEV, "%s",
			    "udev_enumerate_scan_devices failed");
		goto out;
	}

	list_entries = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(list_entry, list_entries) {
		const char *path;
		struct udev_device *dev;
		char **devnodev_new;
		const char *devnode;

		path = udev_list_entry_get_name(list_entry);

		dev = udev_device_new_from_syspath(main_udev, path);
		if (!dev) {
			evb_err_set(main_err, EVB_ERR_NUM_UDEV, "%s",
				    "failed to create udev device");
			goto out;
		}

		if (strncmp(udev_device_get_sysname(dev), "event", 5))
			goto continue_loop;

		devnodev_new = realloc(devnodev,
				       sizeof(char *) * (devnodec + 1));
		if (!devnodev_new) {
			evb_err_set(main_err, EVB_ERR_NUM_SYS, "%s",
				    "failed to allocate memory for devnodes");
			udev_device_unref(dev);
			goto out;
		}
		devnodev = devnodev_new;

		devnode = udev_device_get_devnode(dev);
		devnodev[devnodec] = malloc(sizeof(char) *
					    (strlen(devnode) + 1));
		if (!devnodev[devnodec]) {
			evb_err_set(main_err, EVB_ERR_NUM_SYS, "%s",
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
	if (devnodev && evb_err_num(main_err)) {
		for (size_t i = 0; i < devnodec; ++i) {
			free(devnodev[i]);
		}
		free(devnodev);
		devnodev = NULL;
	}

	if (enumerate)
		udev_enumerate_unref(enumerate);

	return devnodev;
}

static int main_loop()
{
	int retval = -1;
	char **evdevs;
	size_t evdev_count;

	evdevs = main_get_evdevs(&evdev_count);
	if (!evdevs)
		goto out;

	for (size_t i = 0; i < evdev_count; ++i) {
		int fd = open(evdevs[i], O_RDONLY);
		if (fd == -1) {
			syslog(LOG_WARNING, "failed to open %s: %s",
			       evdevs[i], strerror(errno));
		} else {
			FD_SET(fd, &main_evdev_fds);
			main_evdev_max_fd = max(fd, main_evdev_max_fd);
		}
		free(evdevs[i]);
	}

	while (!main_is_stopped) {
		pause();
	}

	retval = 0;
out:
	free(evdevs);

	return retval;
}

static void main_sighandler(const int __attribute__ ((unused)) signum)
{
	main_is_stopped = 1;
}

static int main_set_sighandlers()
{
	struct sigaction sigact;
	sigset_t sighandler_mask;

	sigfillset(&sighandler_mask);

	sigact.sa_handler = &main_sighandler;
	sigact.sa_mask = sighandler_mask;
	sigact.sa_flags = 0;

	if (sigaction(SIGUSR1, &sigact, NULL))
		return -1;

	if (sigaction(SIGUSR2, &sigact, NULL))
		return -1;

	if (sigaction(SIGHUP, &sigact, NULL))
		return -1;

	if (sigaction(SIGINT, &sigact, NULL))
		return -1;

	if (sigaction(SIGTERM, &sigact, NULL))
		return -1;

	return 0;
}

int main(int argc, char **argv)
{
	int exitval = EXIT_FAILURE;

	main_parse_args(argc, argv);

	if (main_init())
		return EXIT_FAILURE;

	if (!main_no_daemon && main_daemonize())
		goto out;

	if (main_set_sighandlers())
		goto out;

	if (main_loop())
		goto out;

	exitval = EXIT_SUCCESS;
out:
	if (evb_err_num(main_err))
		syslog(LOG_ERR, "%s", evb_err_str(main_err));

	main_free();

	return exitval;
}
