GKI Tools for Android Kernels
=============================

Overview
--------

This directory contains helpful tools that may be used to aid in the
development of modularized drivers.

add_EXPORT_SYMBOLS_GPL
----------------------

    USAGE: add_EXPORT_SYMBOL_GPL [--no-skip-arch] < kernel_build_error_log
           add_EXPORT_SYMBOL_GPL [--no-skip-arch] kernel_build_error_log
           grep /<module>[.]ko build_error_log | add_EXPORT_SYMBOL_GPL [--no-skip-arch]
           vi `add_EXPORT_SYMBOL_GPL [--no-skip-arch] < kernel_build_error_log`

To acquire the kernel_build_error_log eg:

    $ ./build_sm8250.sh -j50 2>&1 | tee kernel_build_error_log

To only create commit related to symbols needed for cam_spec.ko module:

    $ grep /cam_spec[.]ko kernel_build_error_log | add_EXPORT_SYMBOL_GPL

To only create commit related to a specific list of symbols, there is
the option to land just the symbols, no spaces, one per line, into a
manufactured or edited kernel_build_error_log and feed that to the script.

The script will only affect the current directory level and downward,
this allows one to segregate the adjusted content.  Any symbols that
are needed outside the range of that directory will result in errors
and the git commit phase will not be performed.

Add EXPORT_SYMBOL_GPL for any noted missing symbols, output the list of files
modified to stdout (so it can be passed to an editor command line should you
need to check or adjust the results). Automatically commit the list of files
into git.

Deals as simply as it can to handle \_\_trace\_\<symbols>, sorting the result.

Keep in mind exports can change, be added or subtracted, and that preliminary
work may expose or remove required symbols to resolve during later work.  As
such this script only adds, so you may need to revert the results and try
again to get the most up to date set.  By making this part automated it can
deal with the tens or thousands of exports that need to be discovered or
added.  If you need to adjust a subsystem, run this script in the subsystem
directory, and it will only adjust from that point downwards leaving other
higher up trees alone.

add_MODULE_LICENSE
------------------

    USAGE: add_MODULE_LICENSE < kernel_build_error_log
           add_MODULE_LICENSE kernel_build_error_log

Add MODULE_LICENSE to all the files.

Must be performed in the root directory.

find_circular
-------------

    USAGE: find_circular [dir]

Call this when depmod breaks down, or when one needs a list of the symbols
implicated in the circular dependency.

Search current or dir directory for all kernel modules.  Itemize what they
export, and what they import.  Discover links and report who fulfills them.
Report any first order circular relationships and the symbols that got us
into the situation.

Standard output is of the form:

module1.ko(symbols) -> module2.ko(symbols) -> module1.ko

Leaves an annotated modules.dep file in the specified directory.

device_snapshot
---------------

    USAGE: device_snapshot [-s <serialno>] [-D] [-f [<input>]] [-F [-o <output> [-d <input>]]]

Collect filtered /dev and /sys details, along with dmesg and probe list.

-o \<output> will drop the collection into a set of files, but will not
overrite existing content.  -F will overwrite.

-D will wait for the display

if \<output> is empty ('' or last option), will not collect dmesg or probe
list.  If no -o option is specified, then \<output> will be default of -
(stdout) and all pieces will go to the standard output separated by a cut
and snip header.  If specified, \<output> will contain the filtered /dev/
and /sys/ dumps, \<output>.probed the subset filter of just the probed drivers,
\<output>.dmesg the kernel logs and \<output>.config the uncompressed
/proc/config.gz.

-d \<input> will take the dropped collection specified to -o \<output> and
produce a diff -U1 output compared against the \<input>.

-f \<input> allows one to utilize the filter to an existing find /dev /sys
output from a device.  No dmesg will be collected.

-s \<serialno> will allow one to specify a device to connect to when multiples
are available, otherwise will default to one available or ANDROID_SERIAL
environment variable.

In your local build/flash/boot script for tight development cycles, add

    SEQ=`for i in out/${DEFAULT_BUILD}.snapshot.[0-9]*; do
           echo ${i#out/${DEFAULT_BUILD}.snapshot.}
         done |
         sed 's/^0*//' |
         grep -v 0-9 |
         tr -d .[:alpha:] |
         sort -nu |
         tail -1` &&
    NEWSEQ=$((${SEQ:-0}+1)) &&
    NEWSEQ=`printf "%03u" ${NEWSEQ}`
    if [ -z "${SEQ}" ]; then
      private/msm-google/scripts/gki/device_snapshot \
        -o out/${DEFAULT_BUILD}.snapshot.${NEWSEQ}
    else
      SEQ=`printf "%03u" ${SEQ}`
      private/msm-google/scripts/gki/device_snapshot \
        -o out/${DEFAULT_BUILD}.snapshot.${NEWSEQ} \
        -d out/${DEFAULT_BUILD}.snapshot.${SEQ}
    fi

instrument_module_init
----------------------
    USAGE: instrument_module_init [dir|file]

Add debug instrumentation to module_init and probe functions.
