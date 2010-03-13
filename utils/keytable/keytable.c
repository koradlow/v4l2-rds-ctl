/* keytable.c - This program allows checking/replacing keys at IR

   Copyright (C) 2006-2010 Mauro Carvalho Chehab <mchehab@redhat.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <argp.h>

#include "parse.h"

struct keytable {
	int codes[2];
	struct keytable *next;
};

struct uevents {
	char		*key;
	char		*value;
	struct uevents	*next;
};

static int parse_code(char *string)
{
	struct parse_key *p;

	for (p = keynames; p->name != NULL; p++) {
		if (!strcasecmp(p->name, string))
			return p->value;
	}
	return -1;
}

const char *argp_program_version = "IR keytable control version 0.1.0";
const char *argp_program_bug_address = "Mauro Carvalho Chehab <mchehab@redhat.com>";

static const char doc[] = "\nAllows get/set IR keycode/scancode tables\n"
	"You need to have read permissions on /dev/input for the program to work\n"
	"\nOn the options bellow, the arguments are:\n"
	"  DEV     - the /dev/input/event* device to control\n"
	"  SYSDEV  - the ir class as found at /sys/class/irrcv\n"
	"  TABLE   - a file wit a set of scancode=keycode value pairs\n"
	"  SCANKEY - a set of scancode1=keycode1,scancode2=keycode2.. value pairs\n";

static const struct argp_option options[] = {
	{"verbose",	'v',	0,		0,	"enables debug messages", 0},
	{"clear",	'c',	0,		0,	"clears the old table", 0},
	{"sysdev",	's',	"SYSDEV",	0,	"ir class device to control", 0},
	{"device",	'd',	"DEV",		0,	"ir device to control", 0},
	{"get-table",	'g',	0,		0,	"reads the current scancode/keycode table", 0},
	{"put-table",	'p',	"TABLE",	0,	"adds/replaces the scancodes to a new scancode/keycode table", 0},
	{"set-key",	'k',	"SCANKEY",	0,	"Change scan/key pairs", 0},
	{ 0, 0, 0, 0, 0, 0 }
};

static const char args_doc[] =
	"--device [/dev/input/event* device]\n"
	"--sysdev [ir class (f. ex. irrcv0)]\n"
	"[for using the irrcv0 sysdev]";


static char *devclass = "irrcv0";
static char *devname = NULL;
static int read = 0;
static int clear = 0;
static int debug = 0;

struct keytable keys = {
	{0, 0}, NULL
};

struct keytable *nextkey = &keys;

static error_t parse_keyfile(char *fname)
{
	FILE *fin;
	int value;
	char *scancode, *keycode, s[2048];

	fin = fopen(fname, "r");
	if (!fin) {
		perror("opening keycode file");
		return errno;
	}

	while (fgets(s, sizeof(s), fin)) {
		scancode = strtok(s, "\n\t =:");
		if (!scancode) {
			perror ("parsing input file scancode");
			return EINVAL;
		}
		if (!strcasecmp(scancode, "scancode")) {
			scancode = strtok(NULL,"\n\t =:");
			if (!scancode) {
				perror ("parsing input file scancode");
				return EINVAL;
			}
		}

		keycode=strtok(NULL, "\n\t =:(");
		if (!keycode) {
			perror("parsing input file keycode");
			return EINVAL;
		}

		if (debug)
			fprintf(stderr, "parsing %s=%s:", scancode, keycode);
		value=parse_code(keycode);
		if (debug)
			fprintf(stderr, "\tvalue=%d\n",value);

		if (value == -1) {
			value = strtol(keycode, NULL, 0);
			if (errno)
				perror("value");
		}

		nextkey->codes[0] = (unsigned) strtol(scancode, NULL, 0);
		nextkey->codes[1] = (unsigned) value;
		nextkey->next = calloc(1, sizeof(keys));
		if (!nextkey->next)
			return ENOMEM;
		nextkey = nextkey->next;
	}
	fclose(fin);

	return 0;
}

static error_t parse_opt (int k, char *arg, struct argp_state *state)
{
	char *p;
	long key;

	switch (k) {
	case 'v':
		debug++;
		break;
	case 'c':
		clear++;
		break;
	case 'd':
		devname=arg;
		break;
	case 's':
		devclass=arg;
		break;
	case 'g':
		read++;
		break;
	case 'p':
		return parse_keyfile(arg);
	case 'k':
		p = strtok(arg, ":=");
		do {
			if (!p)
				return EINVAL;
			nextkey->codes[0] = strtol(p, NULL, 0);
			if (errno)
				return EINVAL;

			p = strtok(NULL, ",;");
			if (!p)
				return EINVAL;
			key = parse_code(p);
			if (key == -1) {
				key = strtol(p, NULL, 0);
				if (errno)
					return EINVAL;
			}
			nextkey->codes[1] = key;

			nextkey->next = calloc(1, sizeof(keys));
			if (!nextkey->next)
					return ENOMEM;
			nextkey = nextkey->next;

			p = strtok(NULL, ":=");
		} while (p);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {
	.options = options,
	.parser = parse_opt,
	.args_doc = args_doc,
	.doc = doc,
};

static void prtcode (int *codes)
{
	struct parse_key *p;

	for (p=keynames;p->name!=NULL;p++) {
		if (p->value == (unsigned)codes[1]) {
			printf("scancode 0x%04x = %s (0x%02x)\n", codes[0], p->name, codes[1]);
			return;
		}
	}

	if (isprint (codes[1]))
		printf("scancode %d = '%c' (0x%02x)\n", codes[0], codes[1], codes[1]);
	else
		printf("scancode %d = 0x%02x\n", codes[0], codes[1]);
}

static int seek_sysfs_dir(char *dname, char *node_name, char **node_entry)
{
	DIR             *dir;
	struct dirent   *entry;
	int		rc;

	dir = opendir(dname);
	if (!dir) {
		perror(dname);
		return errno;
	}
	entry = readdir(dir);
	while (entry) {
		if (!strncmp(entry->d_name, node_name, strlen(node_name))) {
			*node_entry = malloc(strlen(dname) + strlen(entry->d_name) + 2);
			strcpy(*node_entry, dname);
			strcat(*node_entry, entry->d_name);
			strcat(*node_entry, "/");
			rc = 1;
			break;
		}
		entry = readdir(dir);
	}
	closedir(dir);

	return rc;
}

static void free_uevent(struct uevents *uevent)
{
	struct uevents *old;
	do {
		old = uevent;
		uevent = uevent->next;
		free (old);
	} while (uevent);
}

struct uevents *read_sysfs_uevents(char *dname)
{
	FILE		*fp;
	struct uevents	*next, *uevent;
	char		*event = "uevent", *file, s[4096];

	next = uevent = malloc(sizeof(*uevent));

	file = malloc(strlen(dname) + strlen(event) + 1);
	strcpy(file, dname);
	strcat(file, event);

	if (debug)
		fprintf(stderr, "Parsing uevent %s\n", file);


	fp = fopen (file, "r");
	if (!fp) {
		perror(file);
		free(file);
		return NULL;
	}
	while (fgets(s, sizeof(s), fp)) {
		char *p = strtok(s, "=");
		if (!p)
			continue;
		next->key = malloc(strlen(p) + 1);
		if (!next->key) {
			perror("next->key");
			free(file);
			free_uevent(uevent);
			return NULL;
		}
		strcpy(next->key, p);

		p = strtok(NULL, "\n");
		if (!next->value) {
			fprintf(stderr, "Error on uevent information\n");
			fclose(fp);
			free(file);
			free_uevent(uevent);
			return NULL;
		}
		next->value = malloc(strlen(p) + 1);
		if (!next->value) {
			perror("next->value");
			free(file);
			free_uevent(uevent);
			return NULL;
		}
		strcpy(next->value, p);

		if (debug)
			fprintf(stderr, "%s uevent %s=%s\n", file, next->key, next->value);

		next->next = malloc(sizeof(*next));
		if (!next->next) {
			perror("next->next");
			free(file);
			free_uevent(uevent);
			return NULL;
		}
		next = next->next;
	}
	fclose(fp);
	free(file);

	return uevent;
}

static char *node_input, *node_event;

static char *find_device(void)
{
	struct uevents  *uevent;
	char		dname[256], *name = NULL;
	char		*input = "input", *event = "event";
	int		rc;
	char		*DEV = "/dev/";

	/*
	 * Get event sysfs node
	 */
	snprintf(dname, sizeof(dname), "/sys/class/irrcv/%s/", devclass);

	rc = seek_sysfs_dir(dname, input, &node_input);
	if (rc == 0)
		fprintf(stderr, "Couldn't find input device. Old driver?");
	if (rc <= 0)
		return NULL;
	if (debug)
		fprintf(stderr, "Input sysfs node is %s\n", node_input);

	rc = seek_sysfs_dir(node_input, event, &node_event);
	if (rc == 0)
		fprintf(stderr, "Couldn't find input device. Old driver?");
	if (rc <= 0)
		return NULL;
	if (debug)
		fprintf(stderr, "Event sysfs node is %s\n", node_event);

	uevent = read_sysfs_uevents(node_event);
	if (!uevent)
		return NULL;

	while (uevent->next) {
		if (!strcmp(uevent->key, "DEVNAME")) {
			name = malloc(strlen(uevent->key) + strlen(DEV) + 1);
			strcpy(name, DEV);
			strcat(name, uevent->value);
			break;
		}
		uevent = uevent->next;
	}
	free_uevent(uevent);

	if (debug)
		fprintf(stderr, "input device is %s\n", name);

	return name;
}

int main (int argc, char *argv[])
{
	int fd;
	unsigned int i, j;
	int dev_from_class = 0, done = 0;

	argp_parse (&argp, argc, argv, 0, 0, 0);

	if (!devname) {
		devname = find_device();
		if (!devname)
			return -1;
		dev_from_class++;
	}
	if (debug)
		fprintf(stderr, "Opening %s\n",devname);
	if ((fd = open(devname, O_RDONLY)) < 0) {
		perror(devname);
		return -1;
	}
	if (dev_from_class)
		free(devname);

	/*
	 * First step: clear, if --clear is specified
	 */
	if (clear) {
		int codes[2];

		/* Clears old table */
		for (j = 0; j < 256; j++) {
			for (i = 0; i < 256; i++) {
				codes[0] = (j << 8) | i;
				codes[1] = KEY_RESERVED;
				ioctl(fd, EVIOCSKEYCODE, codes);
			}
		}
		done++;
	}

	/*
	 * Second step: stores key tables from file or from commandline
	 */
	nextkey = &keys;
	while (nextkey->next) {
		int value;
		struct keytable *old;

		if (debug)
			fprintf(stderr, "\t%04x=%04x\n",
			       nextkey->codes[0], nextkey->codes[1]);

		if (ioctl(fd, EVIOCSKEYCODE, nextkey->codes)) {
			fprintf(stderr,
				"Setting scancode 0x%04x with 0x%04x via ",
				nextkey->codes[0], nextkey->codes[1]);
			perror ("EVIOCSKEYCODE");
		}
		old = nextkey;
		nextkey = nextkey->next;
		if (old != &keys)
			free(old);
		done++;
	}

	/*
	 * Third step: display current keytable
	 */
	if (read) {
		done++;
		for (j = 0; j < 256; j++) {
			for (i = 0; i < 256; i++) {
				int codes[2];

				codes[0] = (j << 8) | i;
				if (!ioctl(fd, EVIOCGKEYCODE, codes) && codes[1] != KEY_RESERVED)
					prtcode(codes);
			}
		}
	}

	if (!done) {
		argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
	}

	return 0;
}