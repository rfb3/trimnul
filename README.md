# eliminate_terminal_nulls

eliminate_terminal_nulls is a Unix-style command-line utility that
scans one or more files and removes any trailing NUL ('\0') bytes
found at the ends of those files.

This tool is useful when working with files that have been corrupted
or produced by buggy software that appends unnecessary null bytes at
the end of file.

## Usage

    eliminate_terminal_nulls [OPTIONS] FILE...

### Arguments

FILE... — One or more pathnames to regular files to process.

### Options

  -n, --dry-run     Show what changes would be made without modifying any files.
  -v, --verbose     Print details about each file examined.
  -q, --quiet       Suppress normal output; only report errors.
  -h, --help        Display usage help.
  -V, --version     Display program version.

## Examples

Remove trailing null bytes from all files in the current directory:

    eliminate_terminal_nulls *

Perform a dry run to see what would change:

    eliminate_terminal_nulls --dry-run *.dat

## Exit Codes

  0 — All files processed successfully.  
  1 — One or more files could not be opened or modified.  
  2 — Invalid command-line usage.

## Versioning

The current version of this program is recorded in the file named VERSION
in the same directory as this README.

Version tags in the Git repository follow the convention:

    eliminate_terminal_nulls/vMAJOR.MINOR.PATCH

Example:

    eliminate_terminal_nulls/v1.0.0

Each such tag marks a release of this program only, not of the entire monorepo.

## License

Placed in the Public Domain by Rick Busdiecker on 12 October 2025

## See Also

tr(1), dd(1), hexdump(1), od(1)
