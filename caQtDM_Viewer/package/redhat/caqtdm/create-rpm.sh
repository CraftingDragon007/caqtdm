#!/bin/bash
echo "" 
echo "     caQtDM BuildScript2RPM  "
echo ""
# If you want to compile latest release candidate uncomment this line
REPOSITORY_NAME=caqtdm
REPOSITORY=https://github.com/caqtdm/$REPOSITORY_NAME.git
# BRANCH_OR_TAG=V${CAQTDM_VERSION}
BRANCH_OR_TAG=Development

# Package version: env CAQTDM_VERSION wins, else qtdefs.pri, else 1.0.0
if [ -z "${CAQTDM_VERSION}" ]; then
  CAQTDM_VERSION=$(sed -n 's/^[[:space:]]*CAQTDM_VERSION[[:space:]]*=[[:space:]]*[Vv]\{0,1\}\([0-9][0-9.]*\).*/\1/p' ../../../qtdefs.pri 2>/dev/null | head -n 1)
fi
if [ -z "${CAQTDM_VERSION}" ]; then
  echo "WARNING: could not determine caQtDM version, falling back to 1.0.0"
  CAQTDM_VERSION=1.0.0
fi
PACKAGE_VERSION=${CAQTDM_VERSION}
echo "PACKAGE_VERSION=${PACKAGE_VERSION}"
 
if [ "$1" == "--help" ]; then
  echo "" 
  echo "" 
  echo "Usage: create-rpm.sh [OPTION...]"
  echo "Buildscript for caQtDM on Redhat Linux 8/9 or Fedora to RPM"
  echo "" 
  echo "Examples:" 
  echo "./create-rpm.sh              # Normal git checkout + using spec file from git " 
  echo "./create-rpm.sh --rpmdev     # use the current caqtdm.spec in this directory  " 
  echo "./create-rpm.sh --no-checkout # use the current directory as source, do not checkout from git "
  echo "" 
  echo "" 
  echo "" 
  exit 0
fi

if ! [ -f "./caqtdm-${PACKAGE_VERSION}.tar.gz" ]; then

if [ "$1" != "--no-checkout" ]; then
  #### Clone and build caqtdm sources
  git clone $REPOSITORY
  cd $REPOSITORY_NAME
  # TODO Enable this again for serious builds
  git checkout $BRANCH_OR_TAG
  rm -rf .git
  cd ..
  if [ "$1" != "--rpmdev" ]; then
      mv ./caqtdm.spec "./caqtdm.spec_$(date +"%Y_%m_%d_%I_%M")"
      cp ./caqtdm/caQtDM_Viewer/package/rhel/caqtdm.spec ./
  fi



  find ./caqtdm/caQtDM_Viewer/src -type f | xargs chmod 644
  find ./caqtdm/caQtDM_QtControls/src -type f | xargs chmod 644
  find ./caqtdm/caQtDM_Lib/src -type f | xargs chmod 644
  find ./caqtdm/caQtDM_Plugins -type f | xargs chmod 644

  mv caqtdm caqtdm-${PACKAGE_VERSION}
  tar -czf caqtdm-${PACKAGE_VERSION}.tar.gz ./caqtdm-${PACKAGE_VERSION}
else
  echo "Using current directory as source, not checking out from git"
  currentdir=$(pwd)
  cd ../../../../
  tar -czf caqtdm-${PACKAGE_VERSION}.tar.gz --exclude=.git --exclude=caqtdm-${PACKAGE_VERSION}.tar.gz --transform="s,^\.,caqtdm-${PACKAGE_VERSION}," .
  mv caqtdm-${PACKAGE_VERSION}.tar.gz ${currentdir}/
  cd ${currentdir}
fi
fi

if [ ! -d "$HOME/rpmbuild/SOURCES/" ]; then
   mkdir -p "$HOME/rpmbuild/SOURCES/"
fi

: "${EPICS_BASE_TARGET:=/usr/local/epics/base-7.0.9}"
export EPICS_BASE_TARGET
export CAQTDM_NORPATH=1

cp caqtdm-${PACKAGE_VERSION}.tar.gz  ~/rpmbuild/SOURCES/

cp *patch* ~/rpmbuild/SOURCES/ || true

# Keep the spec version in sync with the resolved package version
sed -i "s/^Version:.*/Version: ${PACKAGE_VERSION}/" ./caqtdm.spec

rpmbuild -ba caqtdm.spec
