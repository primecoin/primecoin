# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# What to do
sign=false
verify=false
build=false

# Systems to build
linux=true
windows=true
osx=false
# Other Basic variables
SIGNER=
VERSION=
commit=false
url=https://github.com/primecoin/primecoin
proc=2
mem=2000
scriptName=$(basename -- "$0")
signProg="gpg --detach-sign"
# TODO: enable signing
commitFiles=false

# Help Message
read -d '' usage <<- EOF
Usage: $scriptName [-c|u|v|b|s|B|o|h|j|m|] signer version

Run this script from the directory containing the primecoin, gitian-builder, gitian.sigs, and bitcoin-detached-sigs.

Arguments:
signer          GPG signer to sign each build assert file
version        Version number, commit, or branch to build. If building a commit or branch, the -c option must be specified

Options:
-c|--commit    Indicate that the version argument is for a commit or branch
-u|--url    Specify the URL of the repository. Default is https://github.com/primecoin/primecoin
-v|--verify     Verify the Gitian build
-b|--build    Do a Gitian build
-s|--sign    Make signed binaries for Windows and Mac OSX
-B|--buildsign    Build both signed and unsigned binaries
-o|--os        Specify which Operating Systems the build is for. Default is lwx. l for linux, w for windows, x for osx
-j        Number of processes to use. Default 2
-m        Memory to allocate in MiB. Default 2000
--setup         Set up the Gitian building environment. Uses Docker.
--detach-sign   Create the assert file for detached signing. Will not commit anything.
--no-commit     Do not commit anything to git
-h|--help    Print this help message
EOF

# Get options and arguments
while :; do
    case $1 in
        # Verify
        -v|--verify)
        verify=true
            ;;
        # Build
        -b|--build)
        build=true
            ;;
        # Sign binaries
        -s|--sign)
        sign=true
            ;;
        # Build then Sign
        -B|--buildsign)
        sign=true
        build=true
            ;;
        # PGP Signer
        -S|--signer)
        if [ -n "$2" ]
        then
        SIGNER=$2
        shift
        else
        echo 'Error: "--signer" requires a non-empty argument.'
        exit 1
        fi
           ;;
        # Operating Systems
        -o|--os)
        if [ -n "$2" ]
        then
        linux=false
        windows=false
        osx=false
        if [[ "$2" = *"l"* ]]
        then
            linux=true
        fi
        if [[ "$2" = *"w"* ]]
        then
            windows=true
        fi
        shift
        else
        echo 'Error: "--os" requires an argument containing an l (for linux), w (for windows), or x (for Mac OSX)'
        exit 1
        fi
        ;;
    # Help message
    -h|--help)
        echo "$usage"
        exit 0
        ;;
    # Commit or branch
    -c|--commit)
        commit=true
        ;;
    # Number of Processes
    -j)
        if [ -n "$2" ]
        then
        proc=$2
        shift
        else
        echo 'Error: "-j" requires an argument'
        exit 1
        fi
        ;;
    # Memory to allocate
    -m)
        if [ -n "$2" ]
        then
        mem=$2
        shift
        else
        echo 'Error: "-m" requires an argument'
        exit 1
        fi
        ;;
    # URL
    -u)
        if [ -n "$2" ]
        then
        url=$2
        shift
        else
        echo 'Error: "-u" requires an argument'
        exit 1
        fi
        ;;
        # Detach sign
        --detach-sign)
            signProg="true"
            commitFiles=false
            ;;
        # Commit files
        --no-commit)
            commitFiles=false
            ;;
        # Setup
        --setup)
            setup=true
            ;;
    *)               # Default case: If no more options then break out of the loop.
             break
    esac
    shift
done

export USE_DOCKER=1

# Get signer
if [[ -n "$1" ]]
then
    SIGNER=$1
    shift
fi

# Get version
if [[ -n "$1" ]]
then
    VERSION=$1
    COMMIT=$VERSION
    shift
fi

# Check that a signer is specified
if [[ $SIGNER == "" ]]
then
    echo "$scriptName: Missing signer."
    echo "Try $scriptName --help for more information"
    exit 1
fi

# Check that a version is specified
if [[ $VERSION == "" ]]
then
    echo "$scriptName: Missing version."
    echo "Try $scriptName --help for more information"
    exit 1
fi

# Add a "v" if no -c
if [[ $commit = false ]]
then
    COMMIT="v${VERSION}xpm"
else
    VERSION=${VERSION:1:5}
fi
echo ${COMMIT}
echo ${VERSION}

# Setup build environment
if [[ $setup = true ]]
then
    sudo apt-get install ruby apache2 git apt-cacher-ng python-vm-builder
    git clone https://github.com/devrandom/gitian-builder.git
    # TODO: enable signing
    # git clone https://github.com/primecoin/gitian.sigs.git
    # git clone https://github.com/primecoin/primecoin-detached-sigs.git    
    pushd ./gitian-builder
    mkdir inputs
    cd inputs
    
    bin/make-base-vm --suite bionic --arch amd64 --docker
    cd ..
    popd
fi

# Set up build
pushd ./primecoin
git fetch
git checkout ${COMMIT}
popd

# Build
if [[ $build = true ]]
then
    # Make output folder
    mkdir -p ./primecoin-binaries/${VERSION}
    
    # Build Dependencies
    echo ""
    echo "Building Dependencies"
    echo ""
    pushd ./gitian-builder    
    mkdir -p inputs
    wget -N  -O inputs/miniupnpc-1.6.tar.gz 'http://miniupnp.tuxfamily.org/files/download.php?file=miniupnpc-1.6.tar.gz'
    wget -N -P inputs 'http://fukuchi.org/works/qrencode/qrencode-3.2.0.tar.bz2'
    wget -N -P inputs 'https://downloads.sourceforge.net/project/boost/boost/1.52.0/boost_1_52_0.tar.bz2'
    wget -N -P inputs 'http://www.openssl.org/source/openssl-1.0.1g.tar.gz'
    wget -N -P inputs 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
    wget -N -P inputs 'https://downloads.sourceforge.net/project/libpng/zlib/1.2.7/zlib-1.2.7.tar.gz'
    wget -N -P inputs 'https://downloads.sourceforge.net/project/libpng/libpng15/older-releases/1.5.12/libpng-1.5.12.tar.gz'
    wget -N -P inputs 'https://download.qt.io/archive/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz'

    # Linux
    if [[ $linux = true ]]
    then
        echo ""
        echo "Compiling ${VERSION} Linux"
        echo ""
        ./bin/gbuild -j ${proc} -m ${mem} --commit primecoin=${COMMIT} ../primecoin/contrib/gitian-descriptors/gitian.yml
        mv build/out//bin/64/primecoind ../primecoin-binaries/${VERSION}/primecoind-${VERSION}-linux
        mv build/out//bin/64/primecoin-qt ../primecoin-binaries/${VERSION}/primecoin-qt-${VERSION}-linux
    fi
    # Windows
    if [[ $windows = true ]]
    then
        echo ""
        echo "Compiling ${VERSION} Windows 64"
        echo ""
        ./bin/gbuild -j ${proc} -m ${mem} --commit primecoin=${COMMIT} ../primecoin/contrib/gitian-descriptors/boost-win64.yml && mv build/out/boost-win64-1.52.0-gitian.zip inputs
        ./bin/gbuild -j ${proc} -m ${mem} --commit primecoin=${COMMIT} ../primecoin/contrib/gitian-descriptors/deps-win64.yml && mv build/out/primecoin-deps-win64-0.0.2.zip inputs
        ./bin/gbuild -j ${proc} -m ${mem} --commit primecoin=${COMMIT} ../primecoin/contrib/gitian-descriptors/qt-win64.yml && mv build/out/qt-win64-4.8.7-gitian-r1.zip inputs
        ./bin/gbuild -j ${proc} -m ${mem} --commit primecoin=${COMMIT} ../primecoin/contrib/gitian-descriptors/gitian-win64.yml
        mv build/out/primecoin-*-win32-setup.exe ../primecoin-binaries/${VERSION}/primecoin-${VERSION}-win64-setup.exe
        mv build/out/primecoin-qt.exe ../primecoin-binaries/${VERSION}/primecoin-qt-${VERSION}-win64.exe
    fi
    popd

        if [[ $commitFiles = true ]]
        then
        # Commit to gitian.sigs repo
            echo ""
            echo "Committing ${VERSION} Unsigned Sigs"
            echo ""
            pushd gitian.sigs
            git add ${VERSION}-linux/${SIGNER}
            git add ${VERSION}-win-unsigned/${SIGNER}
            git commit -a -m "Add ${VERSION} unsigned sigs for ${SIGNER}"
            popd
        fi
fi

# Verify the build
if [[ $verify = true ]]
then
    # Linux
    pushd ./gitian-builder
    echo ""
    echo "Verifying v${VERSION} Linux"
    echo ""
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-linux ../primecoin/contrib/gitian-descriptors/gitian-linux.yml
    # Windows
    echo ""
    echo "Verifying v${VERSION} Windows"
    echo ""
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-unsigned ../primecoin/contrib/gitian-descriptors/gitian-win.yml
    # Mac OSX    
    echo ""
    echo "Verifying v${VERSION} Mac OSX"
    echo ""    
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-unsigned ../primecoin/contrib/gitian-descriptors/gitian-osx.yml
    # Signed Windows
    echo ""
    echo "Verifying v${VERSION} Signed Windows"
    echo ""
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../primecoin/contrib/gitian-descriptors/gitian-osx-signer.yml
    # Signed Mac OSX
    echo ""
    echo "Verifying v${VERSION} Signed Mac OSX"
    echo ""
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../primecoin/contrib/gitian-descriptors/gitian-osx-signer.yml    
    popd
fi

# Sign binaries
if [[ $sign = true ]]
then
    
        pushd ./gitian-builder
    # Sign Windows
    if [[ $windows = true ]]
    then
        echo ""
        echo "Signing ${VERSION} Windows"
        echo ""
        ./bin/gbuild -i --commit signature=${COMMIT} ../primecoin/contrib/gitian-descriptors/gitian-win-signer.yml
        ./bin/gsign -p $signProg --signer $SIGNER --release ${VERSION}-win-signed --destination ../gitian.sigs/ ../primecoin/contrib/gitian-descriptors/gitian-win-signer.yml
        mv build/out/primecoin-*win64-setup.exe ../primecoin-binaries/${VERSION}
        mv build/out/primecoin-*win32-setup.exe ../primecoin-binaries/${VERSION}
    fi
    # Sign Mac OSX
    if [[ $osx = true ]]
    then
        echo ""
        echo "Signing ${VERSION} Mac OSX"
        echo ""
        ./bin/gbuild -i --commit signature=${COMMIT} ../primecoin/contrib/gitian-descriptors/gitian-osx-signer.yml
        ./bin/gsign -p $signProg --signer $SIGNER --release ${VERSION}-osx-signed --destination ../gitian.sigs/ ../primecoin/contrib/gitian-descriptors/gitian-osx-signer.yml
        mv build/out/primecoin-osx-signed.dmg ../primecoin-binaries/${VERSION}/primecoin-${VERSION}-osx.dmg
    fi
    popd

        if [[ $commitFiles = true ]]
        then
            # Commit Sigs
            pushd gitian.sigs
            echo ""
            echo "Committing ${VERSION} Signed Sigs"
            echo ""
            git add ${VERSION}-win-signed/${SIGNER}
            git add ${VERSION}-osx-signed/${SIGNER}
            git commit -a -m "Add ${VERSION} signed binary sigs for ${SIGNER}"
            popd
        fi
fi