#!/usr/bin/env bash
SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
cd $SCRIPT_DIR
mkdir -p $HOME/rpmbuild/SOURCES
cd ..
if [ -f ".gitmodules" ]; then
    echo "Found .gitmodules file. Checking submodule status..."

    # Initialize submodules if they are not initialized
    if ! git submodule status > /dev/null 2>&1; then
        echo "Submodules not initialized. Initializing..."
        git submodule init
        git submodule update --recursive
        echo "Submodules initialized and updated."
    else
        # If submodules are already initialized, just update them
        echo "Submodules already initialized. Updating..."
        git submodule update --init --recursive
        echo "Submodules updated."
    fi
else
    echo "No .gitmodules file found. No submodules to manage."
fi
cd $SCRIPT_DIR
tar --exclude=caqtdm_web.spec -czf $HOME/rpmbuild/SOURCES/caqtdm_web.tar.gz .
rpmbuild -ba caqtdm_web.spec


