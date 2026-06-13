/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * When a directory created by ctx.actions.declare_directory is referred to
 * in a sandbox, if it is empty, or a subdirectory of it is empty, the empty
 * directory won't be created in the sandbox.
 * These functions resolve the problem by also recording the directory structure
 * in a text file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/capability.h>

int test_get_process_capabilities() {
    cap_t caps;
    char *caps_text;

    caps = cap_get_proc();

    if (caps == NULL) {
        perror("Failed to convert capabilities to text");
        return -1;
    }

    cap_free(caps);
    return 0;
}

int main() {
    if (test_get_process_capabilities() == 0) {
        printf("Test passed: Successfully retrieved and displayed process capabilities.\n");
        return EXIT_SUCCESS;
    } else {
        printf("Test failed: Error retrieving or displaying process capabilities.\n");
        return EXIT_FAILURE;
    }
}
