#!/usr/bin/env bash
# Keep the Linux executable startable on the Steam Deck: no glibc symbol newer than Steam
# Runtime 3's 2.31 (SteamOS 3.8 ships 2.41), and the GCC runtimes linked in, because the
# build's gcc-14 is newer than the libstdc++ older SteamOS and distro releases carry.
set -euo pipefail

bin=${1:?Usage: check-linux-abi.sh executable}
needed=$(objdump -p "$bin" | grep NEEDED)
printf '%s\n' "$needed"
newest=$(objdump -T "$bin" | grep -oE 'GLIBC_[0-9]+(\.[0-9]+)+' | sort -uV | tail -n1)
echo "Newest glibc symbol version: $newest"
if [ "$(printf '%s\n' "$newest" GLIBC_2.31 | sort -V | tail -n1)" != GLIBC_2.31 ]; then
    echo "::error::$bin needs $newest; the Steam Runtime 3 floor is GLIBC_2.31"
    exit 1
fi
if grep -qE 'NEEDED +lib(stdc\+\+|gcc_s)' <<<"$needed"; then
    echo "::error::$bin links libstdc++ or libgcc_s dynamically; they should be static"
    exit 1
fi
