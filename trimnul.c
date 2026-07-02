//
// trimnul.c - truncates any NUL (0) bytes off end of file
//

//
// Table of Contents
//

// trimnul.c - truncates any NUL (0) bytes off end of file
// Table of Contents
// Headers, etc
// Function prototypes
// close_or_fail
// cursor_append
// fstat_or_fail
// ftruncate_or_fail
// lseek_or_fail
// lseek_whence_to_string
// main
// malloc_or_fail
// open_flags_to_string - non-reentrant / static state
// open_or_fail
// read_or_fail
// scan_block
// trimnul
// usage
// version

//
// Headers, etc
//

#define _LARGEFILE64_SOURCE

#include <getopt.h>

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

typedef struct Options_struct
{
    int dry_run;
    int verbose;
}
Options;


static char* Program_Name = (char*)NULL;

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

static
const char*
lseek_whence_to_string (int whence);

int
main (int    argument_count,
      char** argument_vector);

static
void*
malloc_or_fail (size_t size);

static
const char*
open_flags_to_string (int flags);

static
int
open_or_fail (char* pathname,
              int flags);

static
ssize_t
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
bool
trimnul (Options options,
         char*   pathname);

static
void
usage (FILE* stream,
       int   exit_status);

static
void
version (void);

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
        fprintf (stderr, "%s:%d: In trimnul,"
                 " close(descriptor=%d) failed, returning %d.\n",
                 __FILE__, __LINE__,
                 descriptor, result);
        exit (EXIT_FAILURE);
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

    if (text_length < (*remaining_pointer))
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
        exit (EXIT_FAILURE);
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
                 __FILE__, __LINE__, descriptor, (long int)length, result);
        exit (EXIT_FAILURE);
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
                 ",offset=%ld,whence=\"%s\") failed, returning %lld.\n",
                 __FILE__, __LINE__, descriptor, (long int)offset,
                 lseek_whence_to_string (whence), (long long int)result);
        exit (EXIT_FAILURE);
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
// main
//

int
main (int    argument_count,
      char** argument_vector)
{
    int getopt_result = 0;

    Options options = {
        .dry_run = 0,
        .verbose = 1,
    };

    Program_Name = argument_vector [0];
    while (true)
    {
        static struct option long_options [] =
            {
                {"dry-run", no_argument, (int*)NULL, 1},
                {"help",    no_argument, (int*)NULL, 0},
                {"quiet",   no_argument, (int*)NULL, 0},
                {"verbose", no_argument, (int*)NULL, 1},
                {"version", no_argument, (int*)NULL, 0},
                {0, 0, 0, 0}
            };

        long_options [0].flag = &(options.dry_run);
        long_options [2].flag = &(options.verbose);
        long_options [3].flag = &(options.verbose);

        int option_index = 0;
        getopt_result = getopt_long (
            argument_count, argument_vector, "hnqvV",
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
            else if (strcmp ("help", long_options [option_index].name) == 0)
            {
                usage (stdout, 0);
            }
            else if (strcmp ("version", long_options [option_index].name) == 0)
            {
                version ();
            }
            else
            {
                printf ("option %s", long_options [option_index].name);
                if (optarg)
                {
                    printf (" with arg %s", optarg);
                }
                printf ("\n");
                usage (stderr, EXIT_FAILURE);
            }
            break;

         case 'h':
            usage (stdout, EXIT_SUCCESS);
            break;

         case 'n':
            options.dry_run = true;
            break;

         case 'q':
            options.verbose = false;
            break;

         case 'v':
            options.verbose = true;
            break;

         case 'V':
            version ();
            break;

         case '?':
            /* getopt_long already printed an error message. */
            usage (stderr, EXIT_FAILURE);
            break;

         default:
            fprintf (stderr, "Unrecognized option: -%c\n", getopt_result);
            usage (stderr, EXIT_FAILURE);
        }
    }

    /* Process any remaining command line arguments (not options). */
    for (; optind < argument_count; ++optind)
    {
        char* argument = argument_vector [optind];

        if (trimnul (options, argument) == 1)
        {
            if (options.verbose)
            {
                printf ("Truncated %s\n", argument);
            }
        }
        else
        {
            if (options.verbose)
            {
                printf ("%s was unchanged\n", argument);
            }
        }
    }

    return 0;
}

//
// malloc_or_fail
//

static
void*
malloc_or_fail (size_t size)
{
    void* result = malloc (size);

    if (result == (void*)NULL)
    {
        perror ("malloc(3): ");
        fprintf (stderr, "%s:%d: In malloc_or_fail, malloc("
                 "size=%zu) failed.\n",
                 __FILE__, __LINE__, size);
        exit (EXIT_FAILURE);
    }
    return result;
}

//
// open_flags_to_string - non-reentrant / static state
//

#define MAYBE_APPEND_FLAG(FLAG)                                 \
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

    switch (flags & O_ACCMODE)
    {
     case O_RDONLY:
        cursor = cursor_append (cursor, "O_RDONLY", &remaining);
        is_first = false;
        break;
     case O_WRONLY:
        cursor = cursor_append (cursor, "O_WRONLY", &remaining);
        is_first = false;
        break;
     case O_RDWR:
        cursor = cursor_append (cursor, "O_RDWR", &remaining);
        is_first = false;
        break;
     default:
        {
            char accmode_buffer [32];
            snprintf (accmode_buffer, sizeof (accmode_buffer),
                      "O_ACCMODE=0x%x", flags & O_ACCMODE);
            cursor = cursor_append (cursor, accmode_buffer, &remaining);
            is_first = false;
        }
        break;
    }

    MAYBE_APPEND_FLAG (O_APPEND);
    MAYBE_APPEND_FLAG (O_ASYNC);
    MAYBE_APPEND_FLAG (O_CLOEXEC);
    MAYBE_APPEND_FLAG (O_CREAT);
    // MAYBE_APPEND_FLAG (O_DIRECT);
    MAYBE_APPEND_FLAG (O_DIRECTORY);
    MAYBE_APPEND_FLAG (O_DSYNC);
    MAYBE_APPEND_FLAG (O_EXCL);
    // MAYBE_APPEND_FLAG (O_EXEC);
#ifndef __APPLE__  // Linux, etc.
    MAYBE_APPEND_FLAG (O_LARGEFILE);
#endif // __APPLE__  // Linux, etc.
    MAYBE_APPEND_FLAG (O_NDELAY);
    // MAYBE_APPEND_FLAG (O_NOATIME);
    MAYBE_APPEND_FLAG (O_NOCTTY);
    MAYBE_APPEND_FLAG (O_NOFOLLOW);
    MAYBE_APPEND_FLAG (O_NONBLOCK);
    // MAYBE_APPEND_FLAG (O_PATH);
#ifndef __APPLE__  // Linux, etc.
    MAYBE_APPEND_FLAG (O_RSYNC);
#endif // __APPLE__  // Linux, etc.
    MAYBE_APPEND_FLAG (O_SYNC);
    // MAYBE_APPEND_FLAG (O_TMPFILE);
    MAYBE_APPEND_FLAG (O_TRUNC);

    return result;
}

#undef MAYBE_APPEND_FLAG

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
        exit (EXIT_FAILURE);
    }
    return result;
}

//
// read_or_fail
//

static
ssize_t
read_or_fail (int    descriptor,
              void*  block,
              size_t count)
{
    ssize_t result = read (descriptor, block, count);

    if (result < 0 || (size_t)result != count)
    {
        perror ("read(2): ");
        fprintf (stderr,
                 "%s:%d: In trimnul, "
                 "read(2) returned %zd when expecting %zu.\n",
                 __FILE__, __LINE__, result, count);
        exit (EXIT_FAILURE);
    }

    return result;
}

//
// scan_block
//
// Returns the offset to the file position just after the last
// non-null character in this block, that is, the offset of the last
// character that is not an ASCII NUL (0 byte) character.
//
// If no such character exists, that is, if every byte in the block is
// 0, returns -1.
//

static
off_t
scan_block (int    descriptor,
            off_t  offset,
            char*  block,
            size_t size)
{
    int   index  = -1;
    off_t result = -1;

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

//
// trimnul
//

static
bool
trimnul (Options options,
         char*   pathname)
{
    char*         block                     = (char*)NULL;
    int           descriptor                = (int)0;
    blksize_t     file_block_size           = (off_t)0;
    off_t         file_bytes                = (off_t)0;

    uint_fast64_t file_complete_block_count = (uint_fast64_t)0;
    size_t        file_last_block_size      = (int)0;
    off_t         null_offset               = (off_t)(-1);
    int           open_flags                = O_RDWR;
    bool          result                    = false;
    struct stat   status_buffer;

#ifndef __APPLE__  // Linux, etc.
    open_flags |= O_LARGEFILE;
#endif

    descriptor = open_or_fail (pathname, open_flags);
    (void)fstat_or_fail (descriptor, &status_buffer);

    file_bytes                = status_buffer.st_size;
    file_block_size           = status_buffer.st_blksize;
    file_complete_block_count = file_bytes / file_block_size;
    file_last_block_size      = file_bytes % file_block_size;

    block = (char*)(malloc_or_fail (file_block_size));

    // First check whether the last non-null character is in an incomplete final
    // block of the file
    //
    if (file_last_block_size != 0)
    {
        off_t scan_result
            = scan_block (descriptor,
                          file_block_size * file_complete_block_count,
                          block,
                          file_last_block_size);
        if (scan_result >= 0)
        {
            null_offset = scan_result;
        }
    }

    // If the last non-null character was not found in a final
    // incomplete block, then walk backward through the complete
    // blocks, looking for one containing the last non-null character
    // in the file.
    //
    if (null_offset < 0)
    {
        int_fast64_t block_index = 0;

        for (block_index = file_complete_block_count - 1;
             block_index >= 0;
             --block_index)
        {
            off_t scan_result = scan_block (descriptor,
                                            block_index * file_block_size,
                                            block,
                                            file_block_size);
            if (scan_result >= 0)
            {
                null_offset = scan_result;
                break;
            }
        }
    }

    if (null_offset == -1)
    {
        null_offset = 0;
    }

    if (null_offset != file_bytes)
    {
        result = true;

        if (options.dry_run == 0)
        {
            (void)ftruncate_or_fail (descriptor, null_offset);
        }
    }

    free (block);
    (void)close_or_fail (descriptor);

    return result;
}

//
// usage
//

static
void
usage (FILE* stream,
       int   exit_status)
{
    fprintf (stream, "Usage:\n     %s [OPTIONS] FILE...\n\n", Program_Name);
    fprintf (stream, "Arguments:\n     FILE... — One or more pathnames to regular files to process.\n\n");
    fprintf (stream, "Options:\n");
    fprintf (stream, "    -n, --dry-run     Show what changes would be made without modifying any files.\n");
    fprintf (stream, "    -v, --verbose     Print details about each file examined. [DEFAULT]\n");
    fprintf (stream, "    -q, --quiet       Suppress normal output; only report errors.\n");
    fprintf (stream, "    -h, --help        Display usage help.\n");
    fprintf (stream, "    -V, --version     Display program version.\n");

    exit (exit_status);
}

//
// version
//

static
void
version (void)
{
    printf ("%s, Version 1.0.4\n", Program_Name);
    exit (EXIT_SUCCESS);
}
