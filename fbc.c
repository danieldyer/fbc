/*
 *  Linux Frame Buffer Device Colour Map Configuration
 *
 *  © Copyright 2011 Daniel Dyer
 *			(daniel_dyer@axent.com.au)
 *
 *  --------------------------------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of the Linux
 *  distribution for more details.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include "fb.h"

#define VERSION	"Linux Frame Buffer Device Colour Map Configuration " \
				"Version 1.0 (19/07/2011)\n"  \
				"(C) Copyright 2011 Daniel Dyer\n"

#define DEFAULT_FRAMEBUFFER	"/dev/fb0"

#define DEFAULT_DEPTH 256

// command-line options

static const char *program_name;

static int opt_version = 0;
static int opt_verbose = 0;

static const char *opt_device = NULL;
static const char *opt_r = NULL;
static const char *opt_g = NULL;
static const char *opt_b = NULL;
static const char *opt_depth = NULL;
static const char *opt_gamma = NULL;

static struct {
    const char *name;
    const char **value;
} options[] = {
    { "-d", &opt_device },
    { "-r", &opt_r },
    { "-g", &opt_g },
    { "-b", &opt_b },
    { "-n", &opt_depth },
    { NULL, NULL }
};

// function prototypes

static void usage(void);
int main(int argc, char *argv[]);


// print error message and exit

void die(const char *fmt, ...)
{
    va_list ap;

    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    
    exit(1);
}

// calculate the colour map

static void calculate_cmap(struct fb_cmap *cmap)
{
	float gamma = 0;
	unsigned long depth = DEFAULT_DEPTH;
	unsigned long r = 0;
	unsigned long g = 0;
	unsigned long b = 0;
	int i, temp;
	
	if(opt_gamma)
		gamma = strtof(opt_gamma, NULL);
	
	if(!opt_gamma || gamma < 0.001)
		die("Must supply a valid gamma value\n");
		
	if(opt_depth)
		depth = strtoul(opt_depth, NULL, 0);
	
	if(opt_r)
		r = strtoul(opt_r, NULL, 0);
	if(opt_g)
		g = strtoul(opt_g, NULL, 0);
	if(opt_b)
		b = strtoul(opt_b, NULL, 0);
		
	if(r > depth || g > depth || b > depth)
		die("Colour offset too large\n");
	
	cmap->start = 0;
	cmap->len = depth;
	cmap->transp = NULL;
	cmap->red = malloc(depth * sizeof(__u16));
	cmap->green = malloc(depth * sizeof(__u16));
	cmap->blue = malloc(depth * sizeof(__u16));
	
	for(i = 0; i < depth; i++)
	{
		temp = (i - r) < 0 ? 0 : i - r;
		cmap->red[i] = lround(pow(temp / (float)(depth-1), gamma) * (float)(1<<16));
		
		temp = (i - g) < 0 ? 0 : i - g;
		cmap->green[i] = lround(pow(temp / (float)(depth-1), gamma) * (float)(1<<16));
		
		temp = (i - b) < 0 ? 0 : i - b;
		cmap->blue[i] = lround(pow(temp / (float)(depth-1), gamma) * (float)(1<<16));
	}
	
    return;
}

// print usage and exit

static void usage(void)
{
    puts(VERSION);
    die("\nUsage: %s [options] gamma\n\n"
		"Valid options:\n"
		"  General options:\n"
		"    -h, --help         : display this usage information\n"
		"  Frame buffer special device nodes:\n"
		"    -d <device>        : processed frame buffer device\n"
		"                         (default is " DEFAULT_FRAMEBUFFER ")\n"
		"  Colour map depth:\n"
		"    -n <value>         : number of map entries per colour\n"
		"                         (default is %d)\n"
		"  Offsets for colour channels:\n"
		"    -r <value>         : offset for red channel\n"
		"    -g <value>         : offset for green channel\n"
		"    -b <value>         : offset for blue channel\n",
		program_name, DEFAULT_DEPTH);
}

int main(int argc, char *argv[])
{
    struct fb_cmap cmap;
    int fh = -1, i;

    program_name = argv[0];

	// parse command-line options

    while (--argc > 0) {
		argv++;
		if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help"))
			usage();
		else if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose"))
			opt_verbose = 1;
		else if (!strcmp(argv[0], "-V") || !strcmp(argv[0], "--version"))
			opt_version = 1;
		else {
			for (i = 0; options[i].name; i++)
				if (!strcmp(argv[0], options[i].name))
					break;
			if (options[i].name) {
				if (argc-- > 1) {
					*options[i].value = argv[1];
					argv++;
				} else
					usage();
			} else if (!opt_gamma) {
				opt_gamma = argv[0];
			} else
				usage();
		}
    }

    if (opt_version || opt_verbose)
		puts(VERSION);

    if (!opt_device)
		opt_device = DEFAULT_FRAMEBUFFER;
		
	// calculate and set the colour map

	calculate_cmap(&cmap);

	// open the framebuffer device

    if (opt_verbose)
		printf("Opening framebuffer device '%s'\n", opt_device);

    if ((fh = open(opt_device, O_RDONLY)) == -1)
		die("open %s: %s\n", opt_device, strerror(errno));
    
	// apply the colour map using ioctl

	if (ioctl(fh, FBIOPUTCMAP, &cmap))
		die("ioctl FBIOPUTCMAP: %s\n", strerror(errno));

	free(cmap.red);
	free(cmap.green);
	free(cmap.blue);

    close(fh);

    exit(0);
}
