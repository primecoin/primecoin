{ pkgs, lib, config, inputs, ... }:

let
  # Import packages from old source to ensure compiler and library versions match exactly
  pkgs-old = import inputs.nixpkgs-old {
    system = pkgs.stdenv.hostPlatform.system;
    config = { 
      allowBroken = true; 
      permittedInsecurePackages = [ 
        "openssl-1.0.2u" 
        "db-4.8.30"
      ]; 
    };
  };

  # Use unified toolchain and libraries to prevent GLIBC conflicts
  boost = pkgs-old.boost160;
  openssl = pkgs-old.openssl_1_0_2;
  db48 = pkgs-old.db48;
  # Use GCC and core libraries
  old-gcc = pkgs-old.gcc;
  old-stdenv = pkgs-old.stdenv;
in
{
  # Environment variables: only for compilation phase, do not set global LD_LIBRARY_PATH
  env.BOOST_LIB_PATH = "${boost}/lib";
  env.BOOST_INCLUDE_PATH = "${boost}/include";
  env.OPENSSL_INCLUDE_PATH = "${openssl.dev}/include";
  env.OPENSSL_LIB_PATH = "${openssl.out}/lib";

  # Basic dependencies
  packages = [
    # Build tools
    old-gcc
    pkgs-old.autoconf
    pkgs-old.automake
    pkgs-old.libtool
    pkgs-old.pkg-config
    pkgs-old.gnumake
    pkgs.patchelf  # For fixing binary RPATH
    
    # Core libraries
    boost
    openssl
    db48
    pkgs-old.zlib
    pkgs-old.libevent
    pkgs-old.miniupnpc
    
    # General tools
    pkgs.python3
    pkgs.git
  ];

  # Build script
  scripts.build.exec = ''
    set -e
    echo "🚀 Starting Primecoin build..."
    
    # Thoroughly clean to prevent incompatible configuration residues
    if [ -f Makefile ]; then
      echo "🧹 Cleaning old build files..."
      # Try to clean secp256k1 first (if configured)
      (cd src/secp256k1 && make distclean 2>/dev/null || true) || true
      # Then clean main project
      make distclean 2>/dev/null || true
    fi

    if [ ! -f configure ]; then
      ./autogen.sh
    fi
    
    # Construction of RPATH: Directly burn the library path into the binary file
    # Thus, when running the program, there is no need to set LD_LIBRARY_PATH to find the correct old version of Glibc
    # Note: The library files of Berkeley DB are in the db-4.8.30 package, not db48 (bin package)
    DB_LIB_PATH=$(nix-store -q --references ${db48} | grep "db-4.8.30" | grep -v "bin\|dev" | head -1)
    if [ -z "$DB_LIB_PATH" ] || [ ! -f "$DB_LIB_PATH/lib/libdb_cxx-4.8.so" ]; then
      # If not found, use known path
      DB_LIB_PATH="/nix/store/1y3yldagb7wqaxjcvnjq9pbiv9cyayhn-db-4.8.30"
    fi
    
    RPATH_DIRS=(
      "${openssl.out}/lib"
      "${boost}/lib"
      "${pkgs-old.zlib}/lib"
      "${pkgs-old.libevent}/lib"
      "${pkgs-old.miniupnpc}/lib"
      "$DB_LIB_PATH/lib"
      "${old-gcc.cc.lib}/lib"
      "${pkgs-old.glibc}/lib"
    )
    # Convert array to colon-separated string
    RPATH=$(IFS=:; echo "''${RPATH_DIRS[*]}")

    echo "⚙️  Running configure..."
    ./configure \
      --with-gui=no \
      --with-boost=${boost} \
      --with-boost-libdir=${boost}/lib \
      --with-libdb-prefix="$DB_LIB_PATH" \
      CPPFLAGS="-I${openssl.dev}/include -I${boost}/include -I${pkgs-old.libevent.dev}/include -I$DB_LIB_PATH/include" \
      LDFLAGS="-L${openssl.out}/lib -L${boost}/lib -L${pkgs-old.zlib}/lib -L$DB_LIB_PATH/lib -Wl,-rpath,$RPATH" \
      --disable-tests \
      --disable-bench

    echo "🔨 Compiling (using $(nproc) cores)..."
    # Must explicitly pass LDFLAGS during make to ensure RPATH is injected into binaries
    make -j$(nproc) LDFLAGS="-L${openssl.out}/lib -L${boost}/lib -L${pkgs-old.zlib}/lib -L$DB_LIB_PATH/lib -Wl,-rpath,$RPATH"
    
    echo "🔧 Forcing RPATH fix for binaries (ensuring all dependencies can be found)..."
    TARGETS=("src/primecoind" "src/primecoin-cli" "src/primecoin-tx")
    for bin in "''${TARGETS[@]}"; do
      if [ -f "$bin" ]; then
        echo "  Fixing: $bin"
        # Use --force-rpath to ensure RPATH takes priority over system library paths
        # Use dynamically constructed RPATH (shell variable, not Nix interpolation)
        patchelf --force-rpath --set-rpath "$RPATH" "$bin" 2>/dev/null || echo "    ⚠️  Cannot fix RPATH for $bin"
      fi
    done
    
    echo "✅ Build complete!"
    echo "Library paths injected via RPATH, no LD_LIBRARY_PATH needed to run."
  '';

  # Start service with devenv up
  processes.primecoind.exec = ''
    if [ ! -f src/primecoind ]; then
      echo "❌ Error: src/primecoind not found, please run 'build' first"
      exit 1
    fi
    
    echo "🚀 Starting primecoind..."
    # Clean all path variables that might cause pollution, execute directly
    unset LD_LIBRARY_PATH
    exec ./src/primecoind -printtoconsole"$@"
  '';

  scripts.clean.exec = "make clean || true";
  scripts.distclean.exec = ''
    # Try to clean secp256k1 first (if configured)
    (cd src/secp256k1 && make distclean 2>/dev/null || true) || true
    # Then clean main project
    make distclean 2>/dev/null || true
    # Remove configure script (if complete reconfiguration is needed)
    [ -f configure ] && rm configure && echo "🗑️  Removed configure script" || true
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