#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "debug.h"

#define REBOOT_FILE "/update/factory_reboot"

void do_rfkill(void)
{
	system("killall wpa_supplicant");
	system("killall udhcpc");
	system("rfkill block all");
}

void do_reboot(const char *reason)
{
	int fd;
	time_t tt;
	struct tm *tm;
	char tstr[128] = {0};

	fd = open(REBOOT_FILE, O_RDWR|O_CREAT|O_TRUNC, 0664);
	if (fd >= 0) {
		time(&tt);
		tm = localtime(&tt);
		if (tm) {
			strftime(tstr, sizeof(tstr) - 2, "%H:%M:%S", tm);
			tstr[strlen(tstr) - 1] = '\n';
			write(fd, tstr, strlen(tstr));
		}

		write(fd, reason, strlen(reason));
		write(fd, "\n", 1);
		fsync(fd);
		close(fd);
	}

	system("reboot");
	exit(0);
}

void do_powerdown(void)
{
	system("poweroff");
	exit(0);
}

int get_hardware(char *buf, int len)
{
	int fd;
	char tmp[64] = {0};

	memset(buf, 0, len);
	fd = open("/proc/supernote/machine_tpye", O_RDONLY);
	if (fd < 0) {
		E("open machine type failed,errno=%d(%s)\n",
		  errno, strerror(errno));
		return -1;
	}

	read(fd, tmp, sizeof(tmp) - 1);
	close(fd);
	fd = -1;
	if (!strncmp(tmp, "ratta-sn100",
		     strlen("ratta-sn100"))) {
		strncpy(buf, "ratta-sn100", len - 1);
		return 0;
	} else if (!strncmp(tmp, "ratta-sn078",
			    strlen("ratta-sn078"))) {
		strncpy(buf, "ratta-sn078", len - 1);
		return 0;
	}

	return -1;
}
