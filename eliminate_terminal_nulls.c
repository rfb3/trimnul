//
// eliminate_terminal_nulls.c - truncates any NUL (0) bytes off end of file
//

//
// Table of Contents
//

// eliminate_terminal_nulls.c - truncates any NUL (0) bytes off end of file
// Table of Contents
// Headers, etc
// Function prototypes
// eliminate_terminal_nulls(char*)
// fstat_or_fail (int,struct stat*)
// main(int,char*)
// open_or_fail(char*,int)
// scan_block(int,char*,size_t)

//
// Headers, etc
//

#define _LARGEFILE64_SOURCE

#include <getopt.h>

#include <alloca.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

//
// Function prototypes
//

static
bool
eliminate_terminal_nulls (char* pathname);

static
int
fstat_or_fail (int          descriptor,
               struct stat* status);

int
main (int    argument_count,
      char** argument_vector);

static
int
open_or_fail (char* pathname,
              int flags);

static
off_t
scan_block (int    descriptor,
            off_t  offset,
            char*  block,
            size_t size);

//
// eliminate_terminal_nulls(char*)
//

static
bool
eliminate_terminal_nulls (char* pathname)
{
    char*         block                     = (char*)NULL;
    int           descriptor                = (int)0;
    blksize_t     file_block_size           = (off_t)0;
    off_t         file_bytes                = (off_t)0;

    uint_fast64_t file_complete_block_count = (uint_fast64_t)0;
    size_t        file_last_block_size      = (int)0;
    off_t         null_offset               = (off_t)0;
    bool          result                    = false;
    struct stat   status_buffer;

    descriptor = open_or_fail (pathname, O_RDWR | O_LARGEFILE);
    (void)fstat_or_fail (descriptor, &status_buffer);

    file_bytes                = status_buffer.st_size;
    file_block_size           = status_buffer.st_blksize;
    file_complete_block_count = file_bytes / file_block_size;
    file_last_block_size      = file_bytes % file_block_size;

    /* Note: alloca(3) does not have typical error semantics. Potential updates include:
     *       - use malloc(3)
     *       - hard code an array at least as big as the biggest realistic value of st_blksize
     *       - hard code 4096 instead of st_blksize
     *       - add a SEGV handler around the use of the block buffer
     *
     *       For now, this has the semantics that I want and is unlikely to overflow the stack.
     */
    block = (char*)(alloca (file_block_size));
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
        int_fast64_t block_index = 0;

        for (block_index = file_complete_block_count - 1;
             block_index >= 0;
             --block_index)
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
        result = true;

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

//
// fstat_or_fail (int,struct stat*)
//

static
int
fstat_or_fail (int          descriptor,
               struct stat* status)
{
    int result = fstat (descriptor, status);

    if (result != 0)
    {
        perror ("fstat(2): ");
        fprintf (stderr, "%s:%d: In fstat_or_fail, fstat(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }
    return result;
}

//
// main(int,char*)
//

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
        getopt_result = getopt_long (argument_count, argument_vector, "",
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

//
// open_or_fail(char*,int)
//

static
int
open_or_fail (char* pathname,
              int   flags)
{
    int result = open (pathname, flags);

    if (result < 0)
    {
        perror ("open(2): ");
        fprintf (stderr, "%s:%d: In open_or_fail, open(2) failed.\n",
                 __FILE__, __LINE__);
        exit (1);
    }
    return result;
}

//
// scan_block(int,char*,size_t)
//

static
off_t
scan_block (int    descriptor,
            off_t  offset,
            char*  block,
            size_t size)
{
    int     index        = 0;
    off_t   lseek_result = 0;
    ssize_t read_result  = 0;
    off_t   result       = 0;

    // fprintf (stderr, "%s:%d: scan_block(%d,%ld,0x%lX,%lu)\n",
    //          __FILE__, __LINE__, descriptor, offset, (unsigned long int)block, size);

    lseek_result = lseek (descriptor, offset, SEEK_SET);
    if (lseek_result < 0)
    {
        perror ("lseek(2): ");
        fprintf (stderr,
                 "%s:%d: In eliminate_terminal_nulls, "
                 "lseek(2) failed for offset=%ld.\n",
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
                 "read(2) returned %ld when expecting %lu.\n",
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
