# Logbook of work done on the VC-5 test materials

Work on metadata for VC-5 Part 7 is recorded in a separate [logbook](/Users/brian/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/logbook.md)

GitHub project for work on metadata: [VC-5 Part 2 Revision](https://github.com/SMPTE/smpte-10e-vc5/projects/1)


## Tasks

1. Continue programming command-line arguments for the XML parser using `getopt`


Examples of using `getopt`:
<https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt>


## Notes

In $(ROOT)/external, the argtable software and all header files in the google subdirectory are
checked into GitHub.

The expat library was installed using Homebrew on iMac Desktop.

Where and how was the expat library installed on Windows Home and ZenBook?


### 2019-09-08

Moved release.asc and readme.asc into sub-directory to resolve conflict with the README.md
file used by GitHub.

Installed `asciidoc` and `dblatex` on my iMac Desktop computer but could not resolve problems
with generating PDF from AsciiDoc. Installed `asciidoctor` and the PDF extension and modified
the make file to use `asciidoctor-pdf` instead of `a2x` for generating PDF output.


### 2019-09-09

Why are the include files not listed as dependencies in the dependency files?

For example, `params.d` does not include `params.h`


### 2020-02-05

Checked the binary media files into Git LFS and the non-binary log files into Git.
Updated the working copy of the repository using `git pull` which should have downloaded
the media files from Git.

Used WinMerge to check that the contents of the media folder are the same as would be downloaded
by the `getmedia.py` script. All files are identical. Some folders exist in the working copy, most
likely as a by-product of running the test scripts.


### 2020-02-23

Installed Visual Studio C++ and CMake on my Windows 10 Home virtual machine.

Built the VC-5 encoder. Everything seems to be working okay.


### 2021-04-15

Added README files for the sample encoder and reference decoder.


### 2023-05-25

Preparing to retroactively tag the VC-5 software repository commit that corresponds to the version of RP 2073-2 in the ST Audit package dated 2022-01-12.

Three files were modified since the date of the ST Audit package:
```zsh
% git status
On branch master
Your branch is up to date with 'origin/master'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   metadata/dumper/testcases.dpx
	modified:   metadata/parser/testcases.dpx
	modified:   metadata/python/metrics.csv

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	CMakeLists.txt
	common/common-metrics.csv
	comparer/comparer-metrics.csv
	converter/converter-metrics.csv
	decoder/decoder-metrics.csv
	encoder/debug.txt
	encoder/encoder-metrics.csv
	encoder/results1.txt
	encoder/results2.txt
	metadata/common/common-metrics.csv
	metadata/dumper/dumper-metrics.csv
	metadata/metadata-metrics.csv
	metadata/parser/parser-metrics.csv
	metrics-total.csv
	webpage/
```

The three tracked files not staged for commit were modified after the ST Audit package was created:
```zsh
% ls -l metadata/dumper/testcases.dpx
-rw-r--r--@ 1 brian  staff  3083 Feb 11  2021 metadata/dumper/testcases.dpx

% ls -l metadata/parser/testcases.dpx
-rw-r--r--@ 1 brian  staff  3083 Feb  9  2021 metadata/parser/testcases.dpx

% ls -l metadata/python/metrics.csv
-rw-r--r--@ 1 brian  staff  416 Jun  7  2021 metadata/python/metrics.csv
```

The untracked files are work in progress and not part of the software release corresponding to the ST Audit package.

Only one commit since the ST Audit package was created:
```zsh
% git log | head -40
commit d5fcb173cb9c821fce9e7e79c3d4e15cd94699a3
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Sat Jan 29 12:12:19 2022 -0800

    Ignore Numbers spreadsheets and documentation generated from markdown.

commit 3525b2236246d608c231fce36f1704b61365f1a9
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Mon Jun 7 13:14:49 2021 -0700

    Added targets for computing software metrics.

commit e746528e1d11ea2bf2017b7a4699f411a9353b00
Merge: efeee16 4bf582e
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Mon Jun 7 12:04:22 2021 -0700

    Merge branch 'master' of https://github.com/SMPTE/smpte-10e-vc5

commit efeee161a5104214d572e279976e354f26e42441
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Mon Jun 7 12:02:41 2021 -0700

    Updated the logbook to record addition of the version number to the XML parser and dumper.

commit 4bf582e2effa5120a5b602dd2904b003283982d7
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Tue May 18 14:41:55 2021 -0700

    Adding a feature request template

commit ea0476236d99e047c9604958b6abd116f116068e
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Tue May 18 14:41:15 2021 -0700

    Update issue templates

commit b3b1139db92aca3c6f8f8c7b1659621bf9236949
Author: Brian Schunck <bgschunck@gmail.com>
Date:   Wed Apr 28 11:35:09 2021 -0700
brian@MacBook-M1-Pro smpte-10e-vc5 %
```

The last commit only changed two `.gitignore` files:
```zsh
% git diff-tree --no-commit-id --name-only d5fcb173cb9c821fce9e7e79c3d4e15cd94699a3 -r
.gitignore
metadata/.gitignore
```

Tag the second to last commit:
```zsh
% git tag -a SMPTE-RP-2073-2-2022 3525b2236246d608c231fce36f1704b61365f1a9
% git push origin --tags
```

Changed the tag commit message to
"Tagging the software release before the ST Audit package was created for SMPTE-RP-2073-2:2022"
and pushed the updated tag to the repository:
```zsh
git push origin --tags --force
```


### 2023-08-15

Cloned the VC-5 repository into `~/Projects/SMPTE/VC-5/Temp` and ran the script `install.sh` to download and install external software.

Was able to build the decoder using CMake via the makefile that simplifies the use of CMake:
```zsh
cd $(ROOT)/decoder/build/cmake
make
```

Created debug and release configurations of the decoder.

Tried to run the decoder but could not find the mxml dynamic library:
```zsh
cd debug
./decoder
dyld[44479]: Library not loaded: /usr/local/lib/libmxml.dylib
  Referenced from: <AF11D2FC-9167-3601-87B8-B4D12937319A> /Users/brian/Projects/SMPTE/VC-5/Temp/smpte-10e-vc5/decoder/build/cmake/debug/decoder
  Reason: tried: '/usr/local/lib/libmxml.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/usr/local/lib/libmxml.dylib' (no such file), '/usr/local/lib/libmxml.dylib' (no such file), '/usr/lib/libmxml.dylib' (no such file, not in dyld cache)
zsh: abort      ./decoder
```

Used Homebrew to install `libmxml` but Homebrew does not install the dynamic library in `/user/local` and
could not find where that directory is referenced in the CMakeLists.txt file. The CMakeLists.txt file refers
to the the `mxml` library in `$(EXTERAL)/mxml` so it is not necessary to install `libmxml` using Homebrew.


### 2023-08-20

Worked on fixing a CMake problem: The `libmxml.dyib` dynamic library is not found.
Tried lots of things. Finally, noticed that the dynamic is found if a shell variable is set
before running the program:
```zsh
export DYLD_LIBRARY_PATH=../../../external/mxml
```

Added a command to the CMakeLists.txt file to set the `RPATH` variable:
```CMake
IF(APPLE)
set(CMAKE_BUILD_RPATH ${EXTERNAL}/mxml)
ENDIF()
```

Now the `decoder` can be run and without command-line arguments prints the help message.


### 2023-08-21

The changes made yesterday did not work. Must have had `DYLD_LIBRARY_PATH` set to the location
of the Mini-XML library.

Forced the decoder builds to work (both Linux make files and CMake) by modifying the build files:
`CMakeLlists.txt` and and `decoder.mk`.

Now can build and run the decoder and encoder on macOS using either the CMake build or the
"Linux" build using make files.

TODO: Resolve compiler warnings on macOS.

Tried to specify the RPATH using these [instructions](https://github.com/klee/klee/issues/591)
and the CMake `target_link_options` [declaration](https://cmake.org/cmake/help/latest/command/target_link_options.html)
but could not fix the linker errors from that change.
Stashed the non-working version of `CMakeLists.txt` and reverted to using static library linking on macOS.


### 2023-08-23

Tried to create a Meson build for the decoder. Ran into compiler problems. Even though the `meson.build` file
was adapted from a working make file, there is something wrong with the compilation.

Switched to getting make files to work. Changed the RPATH location of the Mini-XML library.
The decoder builds but when executed cannot find the dynamic library. Can always roll back to
using a static library ore require that `DYLD_LIBRARY_PATH` be set to the directory that contains
the Mini-XML dynamic library.

Able to build and run the encoder and decoder (debug and release configurations) on Ubuntu using the
make file and CMake build methods.

Added table of build status to the README file for the encoder and decoder.

Worked on fixing build problems on Windows. Problems building Mini-XML It does build and
can run the decoder but get no output. Expected the help message.


### 2023-08-24

Could not figure out how to specify the location of the Mini-XML dynamic link library.
Manually copied the DLL to the same directory as the decoder executable and then the decoder
ran as expected.

Cannot change the build files installed with the Mini-XML library. How to automate the copy on Windows?

Figured out how to copy the Mini-XML dynamic library file to the samed directory with the executable program
for both Debug and Release builds on Windows.


### 2023-08-25

Improvements to the scripts for testing the make file builds of the encoder and decoder.

Created a BBEdit project in the root directory for the make files, CMakeLists.txt files,
Bash scripts and logbooks that are edited using BBEdit on macOS.

Source code is edited using Sublime Text using projects in the encoder and decoder subdirectories
but associated scripts and logbooks may be edited using Sublime Text when that program is open.

The `$(ROOT)/scripts` directory contains `testbuild.sh` which is complete but messy script for
testing the encoder builds using make files and CMake.

Cleaned up the encoder and decoder make files by moving the inline Bash code for testing the
builds to separate Bash scripts.

Added a testbuild target to the top-level Makefile but for make file builds only.

TODO: Add CMake builds to the testbuild target in the top-level Makefile.

Finished PowerShell script to test builds of the encoder and decoder (all configurations).

Fixed problem linking to `libexpat.lib` and finding the dynamic link library (DLL) when executing the encoder.

All tests of the CMake build on Windows pass.



