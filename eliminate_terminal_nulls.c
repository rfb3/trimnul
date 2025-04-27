/* eliminate_terminal_nulls.c - truncates any NUL (0) bytes off end of file */

/* Table of Contents */

/* eliminate_terminal_nulls.c - truncates any NUL (0) bytes off end of file */
/* Table of Contents */
/* Headers, etc */
/* Function prototypes */
/* main(int,char*) */
/* eliminate_terminal_nulls(char*) */
/* scan_block(int,char*,size_t) */

/* Headers, etc */

#define _LARGEFILE64_SOURCE

#include <getopt.h>

#include <alloca.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

/* Function prototypes */
static
int
eliminate_terminal_nulls (char* pathname);

int
main (int    argument_count,
      char** argument_vector);

static
int
scan_block (int    descriptor,
            int    offset,
            char*  block,
            size_t size);

/* main(int,char*) */

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
            exit (1);
            break;

         default:
            fprintf (stderr, "Unrecognized option: -%c\n", getopt_result);
            exit (1);
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

/* eliminate_terminal_nulls(char*) */

static
int
eliminate_terminal_nulls (char* pathname)
{
    char*         block                     = (char*)NULL;
    int           descriptor                = (int)0;
    off_t         file_block_size           = 0;
    off_t         file_bytes                = 0;
    int           file_complete_block_count = 0;
    int           file_last_block_size      = 0;
    int           null_offset               = 0;
    int           result                    = (int)0;
    struct stat   status_buffer;

    descriptor = open (pathname, O_RDWR | O_LARGEFILE);
    if (descriptor < 0)
    {
        perror ("open(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, open(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }

    if (fstat (descriptor, &status_buffer) != 0)
    {
        perror ("fstat(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, fstat(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }

    file_bytes                = status_buffer.st_size;
    file_block_size           = status_buffer.st_blksize;
    file_complete_block_count = file_bytes / file_block_size;
    file_last_block_size      = file_bytes % file_block_size;

    block = (char*)(alloca (file_block_size));
    if (block == ((char*)NULL))
    {
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, alloca(3) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }

    if (file_last_block_size != 0)
    {
        int scan_result
            = scan_block (descriptor,
                          file_block_size * file_complete_block_count,
                          block,
                          file_last_block_size);
        if (scan_result > 0)
        {
            null_offset = scan_result;
        }
    }

    if (null_offset == 0)
    {
        int block_index = 0;

        for (block_index = file_complete_block_count - 1;
             block_index >= 0;
             ++block_index)
        {
            int scan_result = scan_block (descriptor,
                                          block_index * file_block_size,
                                          block,
                                          file_block_size);
            if (scan_result > 0)
            {
                null_offset = scan_result;
                break;
            }
        
        }
    }

    if (null_offset != file_bytes)
    {
        result = 1;

        if (ftruncate (descriptor, null_offset) != 0)
        {
            perror ("ftruncate(2): ");
            fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, ftruncate(2) failed.\n",
                     __FILE__, __LINE__);
            exit (1);
        }
    }

    if (close (descriptor) < 0)
    {
        perror ("close(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls, close(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }

    return result;
}

/* scan_block(int,char*,size_t) */

static
int
scan_block (int    descriptor,
            int    offset,
            char*  block,
            size_t size)
{
    int index        = 0;
    int lseek_result = 0;
    int read_result  = 0;
    int result       = 0;

    lseek_result = lseek (descriptor, offset, SEEK_SET);
    if (lseek_result < 0)
    {
        perror ("lseek(2): ");
        fprintf (stderr,
                 "%s:%d: In eliminate_terminal_nulls, "
                 "lseek(2) failed for offset=%d.\n",
                 __FILE__, __LINE__,
                 lseek_result);
        exit (1);
    }

    read_result = read (descriptor,  (void*)block, size);
    if (read_result != size)
    {
        perror ("read(2): ");
        fprintf (stderr,
                 "%s:%d: In eliminate_terminal_nulls, "
                 "read(2) returned %d when expecting %lu.\n",
                 __FILE__, __LINE__, read_result, size);
        exit (1);
    }

    for (index = size - 1; index >= 0; --index)
    {
        if (block [index] != '\0')
        {
            result = offset + index + 1;
            break;
        }
    }
    return result;
}
