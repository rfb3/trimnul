#define _LARGEFILE64_SOURCE

#include <getopt.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

int
main (int    argument_count,
      char** argument_vector);

static
int
eliminate_terminal_nulls (char* pathname);

int
main (int    argument_count,
      char** argument_vector)
{
    int getopt_result;

    while (1)
    {
        static struct option long_options [] =
            {
                {0, 0, 0, 0}
            };

        int option_index = 0;
        getopt_result = getopt_long (argument_count, argument_vector, "abc:d:f:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (getopt_result == -1)
        {
            break;
        }

        switch (getopt_result)
        {
         case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options [option_index].flag != 0)
            {
                break;
            }
            printf ("option %s", long_options [option_index].name);
            if (optarg)
            {
                printf (" with arg %s", optarg);
            }
            printf ("\n");
            break;

         case '?':
            /* getopt_long already printed an error message. */
            break;

         default:
            printf ("Aborting.\n");
            abort ();
        }
    }

    /* Process any remaining command line arguments (not options). */
    for (; optind < argument_count; ++optind)
    {
        char* argument = argument_vector [optind];

        if (eliminate_terminal_nulls (argument) == 1)
        {
            printf ("%s\n", argument);
        }
    }

    return 0;
}

static
int
eliminate_terminal_nulls (char* pathname)
{
    unsigned char character   = (unsigned char)0;
    int           descriptor  = (int)0;
    int           done        = 0;
    off_t         file_bytes  = 0;
    int           null_count  = 0;
    off_t         offset      = (off_t)0;
    ssize_t       read_result = (ssize_t)0;
    int           result      = (int)0;
    struct stat   status_buffer;

    /* printf ("TRACE: open(\"%s\") ... ", pathname); */
    /* fflush (stdout); */
    descriptor = open (pathname, /* O_RDONLY */ O_RDWR | O_LARGEFILE);
    if (descriptor < 0)
    {
        perror ("open(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, open(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }
    /* printf ("done.\n"); */

    /* printf ("TRACE: fstat(%d) ... ", descriptor); */
    /* fflush (stdout); */
    if (fstat (descriptor, &status_buffer) != 0)
    {
        perror ("fstat(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, fstat(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }
    /* printf ("done.\n"); */

    file_bytes = status_buffer.st_size;
    /* printf ("file_bytes = %ld\n", file_bytes); */

    /* This is a quick-n-dirty solution that presumes a relatively
     * small number of terminal NUL characters. It might outright fail
     * in the case of a file made up entirely of NUL characters.
     */
    while (! done)
    {
        /* printf ("TRACE: lseek(%d) ... ", descriptor); */
        /* fflush (stdout); */
        offset = lseek (descriptor, -1 - null_count, SEEK_END);
        /* printf ("done.\n"); */
        if (offset < 0)
        {
            perror ("lseek(2): ");
            fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, lseek(2) failed.\n",
                     __FILE__, __LINE__);
            exit (1);
        }

        /* printf ("TRACE: read(%d) ... ", descriptor); */
        /* fflush (stdout); */
        read_result = read (descriptor, (void*)(&character), 1);
        /* printf ("done.\n"); */
        if (read_result < 0)
        {
            perror ("read(2): ");
            fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, read(2) failed.\n",
                     __FILE__, __LINE__);
            exit (1);
        }
        if (character == ((unsigned char)0))
        {
            /* printf ("character is NUL.\n"); */

            ++null_count;
            if (null_count == file_bytes)
            {
                done = 1;
            }
        }
        else
        {
            /* printf ("character is non-null.\n"); */
            done = 1;
        }
    }
    if (null_count != 0)
    {
        result = 1;

        /* printf ("TRACE: ftruncate(%d,%d) ... ", descriptor, (int)file_bytes - (int)null_count); */
        /* fflush (stdout); */
        if (ftruncate (descriptor, file_bytes - null_count) != 0)
        {
            perror ("ftruncate(2): ");
            fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, ftruncate(2) failed.\n",
                     __FILE__, __LINE__);
            exit (1);
        }
        /* printf ("done.\n"); */
    }

    /* printf ("TRACE: close(%d) ... ", descriptor); */
    /* fflush (stdout); */
    if (close (descriptor) < 0)
    {
        perror ("close(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, close(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }
    /* printf ("done.\n"); */

    return result;
}
