# trimnul

trimnul is a Unix-style command-line utility that
scans one or more files and removes any trailing NUL ('\0') bytes
found at the ends of those files.

This tool is useful when working with files that have been corrupted
or produced by buggy software that appends unnecessary null bytes at
the end of file.

## Usage

    trimnul [OPTIONS] FILE...

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

    trimnul *

Perform a dry run to see what would change:

    trimnul --dry-run *.dat

## Exit Codes

  0 — All files processed successfully.  
  1 — One or more files could not be opened or modified.  
  2 — Invalid command-line usage.

## Versioning

The current version of this program is recorded in the file named VERSION
in the same directory as this README.

Version tags in the Git repository follow the convention:

    trimnul/vMAJOR.MINOR.PATCH

Example:

    trimnul/v1.0.2

Each such tag marks a release of this program only, not of the entire monorepo.

## Repo notes

This was originally called `eliminate_terminal_nulls` and then renamed
to be `trimnul`. Git history tracking is less than ideal here and
--follow is apparently insufficient. However, there _is_ a full
history and git can be compelled to show it with commands like:

```
$ git log --all --graph --decorate --oneline --name-status --find-renames -- c/eliminate_terminal_nulls c/trimnul
```

## License

Placed in the
[Public Domain](https://en.wikipedia.org/wiki/Public_domain)
by Rick Busdiecker on 12 October 2025.

## See Also

[dd(1)](https://www.man7.org/linux/man-pages/man1/dd.1.html),
[hexdump(1)](https://www.man7.org/linux/man-pages/man1/hexdump.1.html),
[od(1)](https://www.man7.org/linux/man-pages/man1/od.1.html),
[tr(1)](https://www.man7.org/linux/man-pages/man1/tr.1.html)
