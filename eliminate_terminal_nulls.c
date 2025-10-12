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
// close_or_fail
// cursor_append
// eliminate_terminal_nulls
// fstat_or_fail
// ftruncate_or_fail
// lseek_or_fail
// lseek_whence_to_string
// open_flags_to_string - non-reentrant / static state
// main
// open_or_fail
// read_or_fail
// scan_block

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
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#define OPEN_FLAGS_TO_STRING_BUFFER_SIZE (1024)

//
// Function prototypes
//

static
int
close_or_fail (int descriptor);

static
char*
cursor_append(char* cursor,
              char* text,
              int*  remaining_pointer);

static
bool
eliminate_terminal_nulls (char* pathname);

static
int
fstat_or_fail (int          descriptor,
               struct stat* status);

static
int
ftruncate_or_fail (int   descriptor,
                   off_t length);

static
off_t
lseek_or_fail (int   descriptor,
               off_t offset,
               int   whence);

int
main (int    argument_count,
      char** argument_vector);

static
int
open_or_fail (char* pathname,
              int flags);

static
int
read_or_fail (int    descriptor,
              void*  block,
              size_t count);

static
off_t
scan_block (int    descriptor,
            off_t  offset,
            char*  block,
            size_t size);

static
const char*
lseek_whence_to_string (int whence);

//
// close_or_fail
//

static
int
close_or_fail (int descriptor)
{
    int result = close (descriptor);

    if (result < 0)
    {
        perror ("close(2): ");
        fprintf (stderr, "%s:%d: In eliminate_terminal_nulls,"
                 " close(descriptor=%d) failed, returning %d.\n",
                 __FILE__, __LINE__,
                 descriptor, result);
        exit (1);
    }

    return result;
}

//
// cursor_append
//

static
char*
cursor_append(char* cursor,
              char* text,
              int*  remaining_pointer)
{
    char* result      = cursor;
    int   text_length = strlen (text);

    if (text_length <= (*remaining_pointer))
    {
        (void)snprintf (result, *remaining_pointer, "%s", text);
        result += text_length;
        (*remaining_pointer) -= text_length;
        
    }
    else
    {
        fprintf (stderr, "cursor_append: Out of buffer space\n");
        fprintf (stderr, "%s:%d: cursor_append("
                 "cursor=\"%s\",text=\"%s\",remaining=%d)",
                 __FILE__, __LINE__, cursor, text, *remaining_pointer);
    }

    return result;
}

//
// eliminate_terminal_nulls
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

        (void)ftruncate_or_fail (descriptor, null_offset);
    }

    (void)close_or_fail (descriptor);

    return result;
}

//
// fstat_or_fail
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
        fprintf (stderr, "%s:%d: In fstat_or_fail, fstat(descriptor=%d"
                 ",status=0x%lX) failed, returning %d.\n",
                 __FILE__, __LINE__,
                 descriptor, (intptr_t)status, result);
        exit (1);
    }
    return result;
}

//
// ftruncate_or_fail
//

static
int
ftruncate_or_fail (int   descriptor,
                   off_t length)
{
    int result = ftruncate (descriptor, length);

    if (result != 0)
    {
        perror ("ftruncate(2): ");
        fprintf (stderr, "%s:%d: In ftruncate_or_fail, ftruncate("
                 "descriptor=%d,length=%ld) failed, returning %d.\n",
                 __FILE__, __LINE__, descriptor, length, result);
        exit (1);
    }
    return result;
}

//
// lseek_or_fail
//

static
off_t
lseek_or_fail (int   descriptor,
               off_t offset,
               int   whence)
{
    off_t result = lseek (descriptor, offset, whence);

    if (result != offset)
    {
        perror ("lseek(2): ");
        fprintf (stderr, "%s:%d: In lseek_or_fail, lseek(descriptor=%d"
                 ",offset=%ld,whence=\"%s\") failed, returning %ld.\n",
                 __FILE__, __LINE__, descriptor, offset,
                 lseek_whence_to_string (whence), result);
        exit (1);
    }
    return result;
}

//
// lseek_whence_to_string
//

static
const char*
lseek_whence_to_string (int whence)
{
    const char* result = "<unknown whence>";
    switch (whence)
    {
     case SEEK_SET:
        result = "SEEK_SET";
        break;
     case SEEK_CUR:
        result = "SEEK_CUR";
        break;
     case SEEK_END:
        result = "SEEK_END";
        break;
    }
    return result;
}

//
// open_flags_to_string - non-reentrant / static state
//

#define EMIT_FLAG(FLAG)                                         \
if ((flags & (FLAG)) != 0)                                      \
{                                                               \
    if (is_first)                                               \
    {                                                           \
        is_first = false;                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        cursor = cursor_append (cursor, "|", &remaining);       \
    }                                                           \
    cursor = cursor_append (cursor, (#FLAG), &remaining);       \
}

static
const char*
open_flags_to_string (int flags)
{
    static char result[OPEN_FLAGS_TO_STRING_BUFFER_SIZE];

    char* cursor    = result;
    bool  is_first  = true;
    int   remaining = OPEN_FLAGS_TO_STRING_BUFFER_SIZE;

    EMIT_FLAG (O_RDONLY);
    EMIT_FLAG (O_WRONLY);
    EMIT_FLAG (O_RDWR);

    EMIT_FLAG (O_APPEND);
    EMIT_FLAG (O_ASYNC);
    EMIT_FLAG (O_CLOEXEC);
    EMIT_FLAG (O_CREAT);
    // EMIT_FLAG (O_DIRECT);
    EMIT_FLAG (O_DIRECTORY);
    EMIT_FLAG (O_DSYNC);
    EMIT_FLAG (O_EXCL);
    // EMIT_FLAG (O_EXEC);
    EMIT_FLAG (O_LARGEFILE);
    EMIT_FLAG (O_NDELAY);
    // EMIT_FLAG (O_NOATIME);
    EMIT_FLAG (O_NOCTTY);
    EMIT_FLAG (O_NOFOLLOW);
    EMIT_FLAG (O_NONBLOCK);
    // EMIT_FLAG (O_PATH);
    EMIT_FLAG (O_RSYNC);
    EMIT_FLAG (O_SYNC);
    // EMIT_FLAG (O_TMPFILE);
    EMIT_FLAG (O_TRUNC);

    return result;
}

#undef EMIT_FLAG

//
// main
//

int
main (int    argument_count,
      char** argument_vector)
{
    int getopt_result;

    printf ("%s\n", open_flags_to_string (O_RDWR | O_APPEND));

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
// open_or_fail
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
        fprintf (stderr, "%s:%d: In open_or_fail, open("
                 "pathname=\"%s\", flags=%s) failed.\n",
                 __FILE__, __LINE__, pathname,
                 open_flags_to_string (flags));
        exit (1);
    }
    return result;
}

//
// read_or_fail
//

static
int
read_or_fail (int    descriptor,
              void*  block,
              size_t count)
{
    int result = read (descriptor, block, count);

    if (result != count)
    {
        perror ("read(2): ");
        fprintf (stderr,
                 "%s:%d: In eliminate_terminal_nulls, "
                 "read(2) returned %d when expecting %lu.\n",
                 __FILE__, __LINE__, result, count);
        exit (1);
    }

    return result;
}

//
// scan_block
//

static
off_t
scan_block (int    descriptor,
            off_t  offset,
            char*  block,
            size_t size)
{
    int   index  = 0;
    off_t result = 0;

    // fprintf (stderr, "%s:%d: scan_block(%d,%ld,0x%lX,%lu)\n",
    //          __FILE__, __LINE__, descriptor, offset, (unsigned long int)block, size);

    (void)lseek_or_fail (descriptor, offset, SEEK_SET);

    (void)read_or_fail (descriptor,  (void*)block, size);

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
