/* toybox-gtests.cpp - Wrapper around scripts/runtest.sh to run each toy test as a gtest
 *
 * Copyright 2023 The Android Open Source Project
 */

#include <dirent.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <functional>
#include <memory>
#include <stdlib.h>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/test_utils.h>

const std::string kShell =
#ifdef __ANDROID__
  _PATH_BSHELL;
#else
  // /bin/sh doesn't work when running on the host, the tests require /bin/bash
  "/bin/bash";
#endif

static void MkdirOrFatal(std::string dir) {
  int ret = mkdir(dir.c_str(), 0777);
  ASSERT_EQ(ret, 0) << "Failed to make directory " << dir << ": " << strerror(errno);
}

static std::string SystemStdoutOrFatal(std::string cmd) {
  CapturedStdout stdout_str;
  int ret = system(cmd.c_str());
  stdout_str.Stop();
  EXPECT_GE(ret, 0) << "Failed to run " << cmd << ": " << strerror(errno);
  EXPECT_EQ(ret, 0) << "Failed to run " << cmd << ": exited with status " << ret;
  return android::base::Trim(stdout_str.str());
}

// ExecTest sets up the environemnt and then execs the toybox test scripts for a single toy.
// It is run in a subprocess as a gtest death test.
static void ExecTest(std::string toy,
                     std::string toy_path,
                     std::string test_file,
                     std::string temp_dir)  {
  std::string test_binary_dir = android::base::GetExecutableDirectory();

  std::string working_dir = temp_dir + "/" + toy;
  MkdirOrFatal(working_dir);

#ifndef __ANDROID__
  std::string path_env = getenv("PATH");
  path_env = temp_dir + "/path:" + path_env;
  setenv("PATH", path_env.c_str(), true);
#endif

  setenv("C", toy_path.c_str(), true);
  setenv("CMDNAME", toy.c_str(), true);
  setenv("TESTDIR", temp_dir.c_str(), true);
  setenv("FILES", (test_binary_dir + "/tests/files").c_str(), true);
  setenv("LANG", "en_US.UTF-8", true);
  setenv("VERBOSE", "1", true);

  std::string test_cmd = android::base::StringPrintf(
    "cd %s && source %s/scripts/runtest.sh && source %s/tests/%s",
    working_dir.c_str(),
    test_binary_dir.c_str(),
    test_binary_dir.c_str(),
    test_file.c_str());

  std::vector<const char*> args;
  args.push_back(kShell.c_str());
  args.push_back("-c");
  args.push_back(test_cmd.c_str());
  args.push_back(NULL);

  // When running in atest something is configure the SIGQUIT signal as blocked, which
  // causes some missed signals in toybox tests that leave dangling "sleep 100" processes
  // lying around.  These processes have the gtest pipe fd open, and keep gtest from
  // considering the death test to have exited until the sleep ends.
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGQUIT);
  sigprocmask(SIG_UNBLOCK, &signal_set, nullptr);

  execv(args[0], const_cast<char**>(args.data()));
  FAIL() << "Failed to exec " << kShell << " -c '" << test_cmd << "'" << strerror(errno);
}

class ToyboxTest : public testing::Test {
 public:
  ToyboxTest(std::string toy, std::string test_file) : toy_(toy), test_file_(test_file) { }
  void TestBody();
 private:
  std::string toy_;
  std::string test_file_;
};


void ToyboxTest::TestBody() {
  // This test function is run once for each toy.
  TemporaryDir temp_dir{};
  bool ignore_failures = false;

#ifdef __ANDROID__
  // On the device, check whether the toy exists
  std::string toy_path = SystemStdoutOrFatal(std::string("which ") + toy_ + " || true");
  if (toy_path.empty()) {
    GTEST_SKIP() << toy_ << " not present";
  }

  // And whether it is uses toybox as its implementation.
  std::string implementation = SystemStdoutOrFatal(std::string("realpath ") + toy_path);
  if (!android::base::EndsWith(implementation, "/toybox")) {
    std::cout << toy_ << " is *not* toybox; this does not count as a test failure";
    // If there is no symlink for the toy on the device then run the tests but don't report
    // failures.
    ignore_failures = true;
  }
#else
  // On the host toybox is packaged with the test so that it can be run in CI, which won't
  // have access to the prebuilt toybox or path symlinks.  It is packaged without any toy
  // symlinks, so a symlink is created for each test.

  // Test if the toy is supported by the packaged toybox, and skip the test if not.
  std::string toybox_path = android::base::GetExecutableDirectory() + "/toybox";
  std::string supported_toys_str = SystemStdoutOrFatal(toybox_path);
  std::vector<std::string> supported_toys = android::base::Split(supported_toys_str, " \n");
  if (std::find(supported_toys.begin(), supported_toys.end(), toy_) == supported_toys.end()) {
    GTEST_SKIP() << toy_ << " not compiled into toybox";
  }

  // Create a directory with a symlinks for all the toys that will be prepended to PATH.
  // Some tests have interdependencies on other toys that may not be available in
  // the host system, e.g. the tar tests depend on the file tool.
  std::string path_dir = std::string(temp_dir.path) + "/path";
  MkdirOrFatal(path_dir);
  for (auto& toy : supported_toys) {
    std::string toy_path = path_dir + "/" + toy;
    int ret = symlink(toybox_path.c_str(), toy_path.c_str());
    ASSERT_EQ(ret, 0) <<
      "Failed to symlink " << toy_path << " to " << toybox_path << ": " << strerror(errno);
  }
  std::string toy_path = path_dir + "/" + toy_;
#endif

  pid_t pid = fork();
  ASSERT_GE(pid, 0) << "Failed to fork";
  if (pid > 0) {
    // parent
    int status = 0;
    int ret = waitpid(pid, &status, 0);
    ASSERT_GT(pid, 0) << "Failed to wait for child " << pid << ": " << strerror(errno);
    ASSERT_TRUE(WIFEXITED(status));
    if (!ignore_failures) {
      int exit_status = WEXITSTATUS(status);
      ASSERT_EQ(exit_status, 0);
    }
  } else {
    // child
    ExecTest(toy_, toy_path, test_file_, temp_dir.path);
    _exit(1);
  }
}

__attribute__((constructor)) static void initTests() {
  // Find all the "tests/*.test" files packaged alongside the gtest.
  std::string test_dir = android::base::GetExecutableDirectory() + "/tests";
  std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(test_dir.c_str()), closedir);
  if (!dir) {
    std::cerr << "Cannot open test executable directory " << test_dir;
    exit(1);
  }

  std::vector<std::string> test_files;
  dirent* de;
  while ((de = readdir(dir.get())) != nullptr) {
    std::string file = de->d_name;
    if (android::base::EndsWith(file, ".test")) {
      test_files.push_back(file);
    }
  }

  std::sort(test_files.begin(), test_files.end());

  // Register each test file as an individual gtest.
  for (auto& test_file : test_files) {
    std::string toy = test_file.substr(0, test_file.size() - strlen(".test"));

    testing::RegisterTest("toybox", toy.c_str(), nullptr, nullptr, __FILE__, __LINE__,
                          [=]() -> ToyboxTest* {
                            return new ToyboxTest(toy, test_file);
                          });
  }
}
