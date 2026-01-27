{ pkgs, lib, config, inputs, ... }:

let
  # Same as 0.16.3: use packages from old nixpkgs
  pkgs-old = import inputs.nixpkgs-old {
    system = pkgs.stdenv.hostPlatform.system;
    config = {
      allowBroken = true;
      permittedInsecurePackages = [ "openssl-1.0.2u" "db-4.8.30" ];
    };
  };
  boost = pkgs-old.boost160;
  openssl = pkgs-old.openssl_1_0_2;
  db48 = pkgs-old.db48;
  old-gcc = pkgs-old.gcc;
in
{
  # Package names and env vars match 0.16.3
  env.BOOST_LIB_PATH = "${boost}/lib";
  env.BOOST_INCLUDE_PATH = "${boost}/include";
  env.OPENSSL_INCLUDE_PATH = "${openssl.dev}/include";
  env.OPENSSL_LIB_PATH = "${openssl.out}/lib";
  env.BDB_INCLUDE_PATH = "${db48.dev}/include";
  env.BDB_LIB_PATH = "${db48.out}/lib";

  packages = [
    old-gcc
    pkgs-old.autoconf
    pkgs-old.automake
    pkgs-old.libtool
    pkgs-old.pkg-config
    pkgs-old.gnumake
    pkgs.patchelf
    boost
    openssl
    db48
    pkgs-old.zlib
    pkgs-old.libevent
    pkgs-old.miniupnpc
    pkgs.python3
    pkgs.git
  ];

  # Build
  scripts.build.exec = ''
    set -e
    echo "Building Primecoin (makefile.unix)..."
    cd src && make -f makefile.unix -j$(nproc) USE_UPNP=- \
      OPENSSL_INCLUDE_PATH="${openssl.dev}/include" \
      OPENSSL_LIB_PATH="${openssl.out}/lib" \
      BOOST_INCLUDE_PATH="${boost}/include" \
      BOOST_LIB_PATH="${boost}/lib" \
      BDB_INCLUDE_PATH="${db48.dev}/include" \
      BDB_LIB_PATH="${db48.out}/lib"
    echo "Done. Binary: src/primecoind"
  '';

  # Start same as 0.16.3: devenv up
  processes.primecoind.exec = ''
    if [ ! -f src/primecoind ]; then
      echo "❌ Error: src/primecoind not found, please run 'build' first"
      exit 1
    fi
    echo "Starting primecoind..."
    unset LD_LIBRARY_PATH
    exec ./src/primecoind -printtoconsole "$@"
  '';

  scripts.clean.exec = ''
    (cd src && make -f makefile.unix clean 2>/dev/null) || true
    echo "Cleaned."
  '';

  enterShell = ''
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Primecoin Build Environment"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "  Build: build"
    echo "  Run: devenv up"
    echo "  Clean: clean"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  '';
}
