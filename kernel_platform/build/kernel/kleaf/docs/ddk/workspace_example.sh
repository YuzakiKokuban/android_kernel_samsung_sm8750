#!/bin/bash -e

# Where you are going to set up the DDK workspace.
WORKSPACE=~/workspace

# Path below $WORKSPACE to store your device-specific code.
DEVICE_CODE_RELPATH=private/mydevice

# A fake remote Git server for the purpose of illustration. You should
# use a real remote Git server for production.
function setup_remote_git_server() {
    GIT_REMOTE_PATH=/tmp/git_projects
    GIT_REMOTE_URL_BASE=file://${GIT_REMOTE_PATH}

    # Set up a repo manifest Git repository. You should do this on a remote Git
    # server; for the purpose of illustration, it is set up locally.
    (
        mkdir -p $GIT_REMOTE_PATH/manifest && cd $GIT_REMOTE_PATH/manifest
        git init
        cat << EOF > default.xml
<?xml version="1.0" ?>
<manifest>
    <remote name="local" fetch="$GIT_REMOTE_URL_BASE" review="$GIT_REMOTE_URL_BASE" />
    <project path="$DEVICE_CODE_RELPATH" name="mydevice" remote="local" revision="main">
        <!-- TODO: Add <linkfile> to top-level files -->
    </project>
</manifest>
EOF
        git add default.xml; git commit -am"Add empty manifest"
    )

    # Set up a Git repository for device-specific code. You should do this on a
    # remote Git server; for the purpose of illustration, it is set up locally.
    (
        mkdir -p $GIT_REMOTE_PATH/mydevice && cd $GIT_REMOTE_PATH/mydevice
        git init
        git commit --allow-empty -m"Initial empty repository"
    )
}
setup_remote_git_server

# Create and enter an empty directory to set up the DDK workspace
mkdir $WORKSPACE && cd $WORKSPACE

# Set up repo manifest. Usually this should point to a remote Git repository.
# For the purpose of illustration, use the local one we just set up.
repo init -u $GIT_REMOTE_URL_BASE/manifest
repo sync -c

# Call init.py to setup the repository properly.
# This runs `repo sync` for you. To skip that and run `repo sync` yourself,
# use --nosync.
curl https://android.googlesource.com/kernel/build/bootstrap/+/refs/heads/main/init.py?format=TEXT | base64 --decode | python3 - \
    --branch aosp_kernel-common-android15-6.6 \
    --ddk_workspace $(realpath .) \
    --kleaf_repo $(realpath .)/external/kleaf \
    --prebuilts_dir $(realpath .)/prebuilts/kernel

# Commit & push any changes made on the manifest
(
    cd .repo/manifests
    git add kleaf.xml default.xml
    git commit -am"Add kleaf projects"
    git push origin HEAD
)

# Version-control top-level files by moving them into the device-specific code
# directory.
# To persist the symlinks, add <linkfile> to the manifest.
for file in $(find . -maxdepth 1 -type f | sed 's/^\.\///g'); do
    mv $file $DEVICE_CODE_RELPATH/$file
    ln -s $DEVICE_CODE_RELPATH/$file $file
done
(
    cd $DEVICE_CODE_RELPATH
    repo start workspace_files
    git add .
    git commit -am"Add workspace files"

    # Upload your changes. You need to set `review` in the manifest properly.
    # repo upload ...
)

# Start developing driver in $DEVICE_CODE_RELPATH
