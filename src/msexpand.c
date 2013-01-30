/*
 *  msexpand: Microsoft "compress.exe/expand.exe" compatible decompressor
 *
 *  Copyright (c) 2000 Martin Hinner <mhi@penguin.cz>
 *  Algorithm & data structures by M. Winterhoff <100326.2776@compuserve.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>

extern char *version_string;

#define N 4096
#define F 16

int
getbyte (int f)
{
  unsigned char b;

  /* Is there a better way?  */
  if (read (f, &b, sizeof (b)) != 1)
    return -1;
  return b;
}

int
expand (int in, char *inname, int out, char *outname)
{
  int bits, ch, i, j, len, mask;
  unsigned char *buffer;

  uint32_t magic1;
  uint32_t magic2;
  uint32_t magic3;
  uint16_t reserved;
  uint32_t filesize;

  if (read (in, &magic1, sizeof (magic1)) != sizeof (magic1))
    {
      perror (inname);
      return -1;
    }

#ifdef WORDS_BIGENDIAN
  if (magic1 == 0x535A4444L)
#else
  if (magic1 == 0x44445A53L)
#endif
    {
      if (read (in, &magic2, sizeof (magic2)) != sizeof (magic2))
	{
	  perror (inname);
	  return -1;
	}

      if (read (in, &reserved, sizeof (reserved)) != sizeof (reserved))
	{
	  perror (inname);
	  return -1;
	}

      if (read (in, &filesize, sizeof (filesize)) != sizeof (filesize))
	{
	  perror (inname);
	  return -1;
	}
#ifdef WORDS_BIGENDIAN
      if (magic2 != 0x88F02733L)
#else
      if (magic2 != 0x3327F088L)
#endif
	{
	  fprintf (stderr, "%s: This is not a MS-compressed file\n", inname);
	  return -1;
	}
    }
  else
#ifdef WORDS_BIGENDIAN
  if (magic1 == 0x4B57414AL)
#else
  if (magic1 == 0x4A41574BL)
#endif
    {
      if (read (in, &magic2, sizeof (magic2)) != sizeof (magic2))
	{
	  perror (inname);
	  return -1;
	}

      if (read (in, &magic3, sizeof (magic3)) != sizeof (magic3))
	{
	  perror (inname);
	  return -1;
	}

      if (read (in, &reserved, sizeof (reserved)) != sizeof (reserved))
	{
	  perror (inname);
	  return -1;
	}

#ifdef WORDS_BIGENDIAN
      if (magic2 != 0x88F027D1L || magic3 != 0x03001200L)
#else
      if (magic2 != 0xD127F088L || magic3 != 0x00120003L)
#endif
	{
	  fprintf (stderr, "%s: This is not a MS-compressed file\n", inname);
	  return -1;
	}
       fprintf (stderr, "%s: Unsupported version 6.22\n", inname);
       return -1;
    }
  else
    {
      fprintf (stderr, "%s: This is not a MS-compressed file\n", inname);
      return -1;
    }


  buffer = malloc (N);
  if (!buffer)
    {
      fprintf (stderr, "%s:No memory\n", inname);
      return -1;
    }

  memset (buffer, ' ', N);

  i = N - F;
  while (1)
    {
      bits = getbyte (in);
      if (bits == -1)
	break;

      for (mask = 0x01; mask & 0xFF; mask <<= 1)
	{
	  if (!(bits & mask))
	    {
	      j = getbyte (in);
	      if (j == -1)
		break;
	      len = getbyte (in);
	      j += (len & 0xF0) << 4;
	      len = (len & 15) + 3;
	      while (len--)
		{
		  buffer[i] = buffer[j];
		  if (write (out, &buffer[i], 1) != 1)
		    {
		      perror (outname);
		      return -1;
		    }
		  j++;
		  j %= N;
		  i++;
		  i %= N;
		}
	    }
	  else
	    {
	      ch = getbyte (in);
	      if (ch == -1)
		break;
	      buffer[i] = ch;
	      if (write (out, &buffer[i], 1) != 1)
		{
		  perror (outname);
		  return -1;
		}
	      i++;
	      i %= N;
	    }
	}
    }

  free (buffer);
  return 0;
}

void
usage (char *progname)
{
  printf ("Usage: %s [-h] [-V] [file ...]\n"
	  " -h --help        give this help\n"
	  " -V --version     display version number\n"
	  " file...          files to decompress. If none given, use"
	  " standard input.\n"
	  "\n"
	  "Report bugs to <mhi@penguin.cz>\n", progname);
  exit (0);
}

int
main (int argc, char **argv)
{
  int in, out;
  char *argv0;
  int c;
  char name[0x100];

  argv0 = argv[0];

  while ((c = getopt (argc, argv, "hV")) != -1)
    {
      switch (c)
	{
	case 'h':
	  usage (argv0);
	case 'V':
          printf ("msexpand version %s " __DATE__ "\n",
                        version_string);
	  return 0;
	default:
	  usage (argv0);
	}
    }

  argc -= optind;
  argv += optind;

  if (argc == 0)
    {
      if (isatty (STDIN_FILENO))
	usage (argv0);
      if (expand (STDIN_FILENO, "STDIN", STDOUT_FILENO, "STDOUT") < 0)
	return 1;
      return 0;
    }

  while (argc)
    {
      if (argv[0][strlen (argv[0]) - 1] != '_')
	{
	  fprintf (stderr, "%s: Doesn't end with underscore -- ignored\n", argv[0]);
	  argc--;
	  argv++;
	  continue;
	}

      in = open (argv[0], O_RDONLY);
      if (in < 0)
	{
	  perror (argv[0]);
	  return 1;
	}

      strcpy (name, argv[0]);
      name[strlen (name) - 1] = 0;

      out = open (name, O_WRONLY | O_CREAT | O_EXCL, 0644);
      if (out < 0)
	{
	  perror (name);
	  return 1;
	}

      expand (in, argv[0], out, name);
      close (in);
      close (out);

      argc--;
      argv++;
    }

  return 0;
}
