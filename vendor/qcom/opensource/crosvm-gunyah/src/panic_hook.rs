// Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// Copyright 2019 The Chromium OS Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use std::env;
use std::fs::File;
use std::io::{stderr, Read};
use std::panic::{self, PanicInfo};
use std::process::abort;
use std::string::String;

use log::error;
use base::{FromRawDescriptor, IntoRawDescriptor};
use libc::{close, dup, dup2, pipe2, O_NONBLOCK, STDERR_FILENO};

// Opens a pipe and puts the write end into the stderr FD slot. On success, returns the read end of
// the pipe and the old stderr as a pair of files.
fn redirect_stderr() -> Option<(File, File)> {
    let mut fds = [-1, -1];
    unsafe {
        // Trivially safe because the return value is checked.
        let old_stderr = dup(STDERR_FILENO);
        if old_stderr == -1 {
            return None;
        }
        // Safe because pipe2 will only ever write two integers to our array and we check output.
        let mut ret = pipe2(fds.as_mut_ptr(), O_NONBLOCK);
        if ret != 0 {
            // Leaks FDs, but not important right before abort.
            return None;
        }
        // Safe because the FD we are duplicating is owned by us.
        ret = dup2(fds[1], STDERR_FILENO);
        if ret == -1 {
            // Leaks FDs, but not important right before abort.
            return None;
        }
        // The write end is no longer needed.
        close(fds[1]);
        // Safe because each of the fds was the result of a successful FD creation syscall.
        Some((
            File::from_raw_descriptor(fds[0]),
            File::from_raw_descriptor(old_stderr),
        ))
    }
}

// Sets stderr to the given file. Returns true on success.
fn restore_stderr(stderr: File) -> bool {
    let descriptor = stderr.into_raw_descriptor();

    // Safe because descriptor is guaranteed to be valid and replacing stderr
    // should be an atomic operation.
    unsafe { dup2(descriptor, STDERR_FILENO) != -1 }
}

// Sends as much information about the panic as possible.
fn log_panic_info(default_panic: &(dyn Fn(&PanicInfo) + Sync + Send + 'static), info: &PanicInfo) {
    // Grab a lock of stderr to prevent concurrent threads from trampling on our stderr capturing
    // procedure. The default_panic procedure likely uses stderr.lock as well, but the mutex inside
    // stderr is reentrant, so it will not dead-lock on this thread.
    let stderr = stderr();
    let _stderr_lock = stderr.lock();

    // Redirect stderr to a pipe we can read from later.
    let (mut read_file, old_stderr) = match redirect_stderr() {
        Some(f) => f,
        None => {
            error!("failed to capture stderr during panic");
            return;
        }
    };
    // Only through the default panic handler can we get a stacktrace. It only ever prints to
    // stderr, hence all the previous code to redirect it to a pipe we can read.
    env::set_var("RUST_BACKTRACE", "1");
    default_panic(info);

    // Closes the write end of the pipe so that we can reach EOF in read_to_string. Also allows
    // others to write to stderr without failure.
    if !restore_stderr(old_stderr) {
        error!("failed to restore stderr during panic");
        return;
    }
    drop(_stderr_lock);

    let mut panic_output = String::new();
    // Ignore errors and print what we got.
    let _ = read_file.read_to_string(&mut panic_output);
    // Split by line because the logging facilities do not handle embedded new lines well.
    for line in panic_output.lines() {
        error!("{}", line);
    }
}

/// The intent of our panic hook is to get panic info and a stacktrace, even for
/// jailed subprocesses. It will always abort on panic to ensure a minidump is generated.
///
/// Note that jailed processes will usually have a stacktrace of <unknown> because the backtrace
/// routines attempt to open this binary and are unable to do so in a jail.
pub fn set_panic_hook() {
    let default_panic = panic::take_hook();
    panic::set_hook(Box::new(move |info| {
        log_panic_info(default_panic.as_ref(), info);
        // Abort to trigger the crash reporter so that a minidump is generated.
        abort();
    }));
}
