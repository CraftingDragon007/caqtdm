#!/bin/bash -e
echo "" 
echo "     caQtDM BuildScript2DEB  "
echo "" 
if [ "$1" == "--help" ]; then
  echo "" 
  echo "" 
  echo "Usage: create-deb.sh [OPTION...]"
  echo "Buildscript for caQtDM on Debian "
  echo "" 
  echo "Examples:" 
  echo "./create-deb.sh              # Normal git checkout + using spec file from git " 
  echo "./create-deb.sh --debdev     # use the current caqtdm.spec in this directory  " 
  echo "" 
  echo "" 
  echo "" 
  exit 0
fi

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

rm -rf caqtdm-${PACKAGE_VERSION}  || true

if [ "$1" != "--debdev" ]; then
  #### Clone and build caqtdm sources
  git clone $REPOSITORY
  cd $REPOSITORY_NAME
  # TODO Enable this again for serious builds
  git checkout $BRANCH_OR_TAG
  rm -rf .git
  cd ..

  mv caqtdm caqtdm_${PACKAGE_VERSION}
  cd caqtdm_${PACKAGE_VERSION}

  tar -czf ../caqtdm_${PACKAGE_VERSION}.orig.tar.gz  --exclude=.git . 
  cd ..
fi


mkdir -p caqtdm-${PACKAGE_VERSION}/
cd caqtdm-${PACKAGE_VERSION}
pwd
dh_make --createorig -p caqtdm_${CAQTDM_VERSION} --packagename caqtdm -s -y || true
pwd
cd ..
pwd
cp -r ./debian/* caqtdm-${PACKAGE_VERSION}/debian/
pwd
cd caqtdm-${PACKAGE_VERSION}
pwd

tar -xf ../caqtdm_${PACKAGE_VERSION}.orig.tar.gz
pwd
cp -rf ../debian/* debian/
rm -rf debian/*.ex debian/README.Debian debian/README.source
pwd

# Keep debian/changelog in sync: prepend an entry if the top version differs
CHANGELOG_VERSION=$(sed -n '1s/^caqtdm (\([0-9][0-9.]*\).*/\1/p' debian/changelog)
if [ "${CHANGELOG_VERSION}" != "${PACKAGE_VERSION}" ]; then
  {
    echo "caqtdm (${PACKAGE_VERSION}-1) unstable; urgency=medium"
    echo ""
    echo "  * Automated build (create-deb.sh)"
    echo ""
    echo " -- Helge Brands <helge.brands@psi.ch>  $(date -R)"
    echo ""
    cat debian/changelog
  } > debian/changelog.new
  mv debian/changelog.new debian/changelog
fi

# Build the package
dpkg-buildpackage -us -uc
