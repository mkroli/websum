/*
 * websum calculates the md5 checksum of the content behind urls
 * Copyright (C) 2008  Michael Krolikowski
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _VERSION_ "0.1"
#define _USER_AGENT_ "websum"
#define _BUFFER_SIZE_ 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <curl/curl.h>
#include <openssl/md5.h>

static MD5_CTX md5;

size_t receive (void* ptr, size_t size, size_t nmemb, void *stream)
{
	MD5_Update (&md5, ptr, size * nmemb);
	return size * nmemb;
}

void version (void)
{
	printf ("websum %s\n", _VERSION_ );
	printf ("Copyright (C) Michael Krolikowski 2008\n");
	printf ("License GPLv2: GNU General Public License Version 2\n");
	printf ("               <http://www.gnu.org/licenses/gpl-2.0.txt>\n");
}

void syntax (char* appname)
{
	printf ("syntax: %s [options] <url1> <url2> ...\n", appname);
	printf ("options:\n");
	printf ("--output <file> - writes output to file\n");
	printf ("-o <file>       - see above\n");
	printf ("--check <file>  - read and check file\n");
	printf ("-c <file>       - see above\n");
	printf ("--version       - prints out information about the version\n");
	printf ("-v              - see above\n");
	printf ("--help          - prints out this text\n");
	printf ("-h              - see above\n");
	printf ("--quiet         - be quiet\n");
}

char* md5sum (char* url)
{
	CURL* curl;
	unsigned char md5sum [16];
	char* result;
	int i;

	result = malloc (sizeof (unsigned char) * 33);
	memset (md5sum, 0, 16);
	memset (result, 0, 33);
	MD5_Init (&md5);
	curl = curl_easy_init ();

	curl_easy_setopt (curl, CURLOPT_URL, url);
	curl_easy_setopt (curl, CURLOPT_USERAGENT, _USER_AGENT_ );
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, receive);

	curl_easy_perform (curl);
	curl_easy_cleanup (curl);

	MD5_Final (md5sum, &md5);

	for (i=0; i<16; i++)
	{
		sprintf (&result[i*2], "%02x", md5sum[i]);
	}
	return result;
}

int main (int argc, char** argv)
{
	int c, i;
	int return_code = EXIT_SUCCESS;
	int quiet = 0;
	char* md5;
	char* output_filename = 0;
	char* input_filename = 0;
	FILE* output = stdout;
	FILE* input = 0;

	if (argc < 2)
	{
		syntax (argv[0]);
		return EXIT_FAILURE;
	}

	while (1)
	{
		static struct option long_options[] =
		{
			{"check",   optional_argument, 0, 'c'},
			{"output",  required_argument, 0, 'o'},
			{"help",    no_argument,       0, 'h'},
			{"version", no_argument,       0, 'v'},
			{"quiet",   no_argument,       0, 'q'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		c = getopt_long (argc, argv, "qhvc::o:", long_options, &option_index);
		if (c == -1)
		{
			break;
		}
		switch (c)
		{
		case 'c':
			if (output_filename)
			{
				fprintf (stderr, "you cannot select --check and --output together\n");
				return EXIT_FAILURE;
			}
			if (optarg)
			{
				input_filename = optarg;
			}
			else
			{
				if (optind<argc)
				{
					input_filename = argv[optind];
				}
				else
				{
					input = stdin;
				}
			}
			break;
		case 'o':
			if (input_filename)
			{
				fprintf (stderr, "you cannot select --check and --output together\n");
				return EXIT_FAILURE;
			}
			output_filename = optarg;
			break;
		case 'v':
			version ();
			return EXIT_SUCCESS;
		case 'h':
			syntax (argv[0]);
			return EXIT_SUCCESS;
		case 'q':
			quiet = 1;
			break;
		case '?':
		default:
			syntax (argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (input || input_filename)
	{
		char buffer [ _BUFFER_SIZE_ ];
		char* line;
		int len;

		if (input_filename)
		{
			input = fopen (input_filename, "r");
		}
		if (!input && input_filename)
		{
			if (!quiet)
			{
				fprintf (stderr, "can't read from \"%s\"\n", input_filename);
			}
			return EXIT_FAILURE;
		}
		while ((line = fgets (buffer, _BUFFER_SIZE_ , input))) {
			len = strlen (line);
			line[len - 1] = 0;
			if (len < 35)
			{
				continue;
			}
			md5 = md5sum (&line[34]);
			if (strncmp (line, md5, 32))
			{
				if (!quiet)
				{
					printf ("%s does not match\n", &line[34]);
				}
				return_code = EXIT_FAILURE;
			}
			else
			{
				if (!quiet)
				{
					printf ("%s matches\n", &line[34]);
				}
			}
			free (md5);
		}
		if (input_filename)
		{
			fclose (input);
		}
	}
	else
	{
		if (output_filename)
		{
			output = fopen (output_filename, "w");
			if (!output)
			{
				if (!quiet)
				{
					fprintf (stderr, "can't write to \"%s\"\n", output_filename);
				}
				return EXIT_FAILURE;
			}
		}
		for (i=optind; i<argc; i++)
		{
			md5 = md5sum (argv[i]);
			fprintf (output, "%s  %s\n", md5, argv[i]);
			free (md5);
		}
		if (output_filename)
		{
			fclose (output);
		}
	}

	return return_code;
}
