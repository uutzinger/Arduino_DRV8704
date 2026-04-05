#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

VERSION="$(awk -F= '$1=="version" {print $2}' library.properties)"
if [[ -z "${VERSION}" ]]; then
  echo "Could not read version from library.properties" >&2
  exit 1
fi

TMP_DOXYFILE="$(mktemp)"
awk -v version="$VERSION" '
  /^PROJECT_NUMBER[[:space:]]*=/ {
    print "PROJECT_NUMBER         = \"" version "\""
    next
  }
  { print }
' Doxyfile > "$TMP_DOXYFILE"
mv "$TMP_DOXYFILE" Doxyfile

doxygen Doxyfile

# Normalize a small set of Doxygen HTML iframe wrappers so repeated runs stay
# idempotent across local generator formatting differences.
find docs -type f -name '*.html' -print0 | while IFS= read -r -d '' file; do
  perl -0pi -e 's#(<div class="center">(?:<div class="zoom">)?<iframe\b[^>]*></iframe>)\s*\n\s*(</div>(?:</div>)?)#$1$2#g' "$file"
done

rm -rf docs/assets
mkdir -p docs/assets

if compgen -G "assets/*" > /dev/null; then
  cp -f assets/* docs/assets/
fi

echo "Docs generated in docs/ for version $VERSION."
