#!/bin/bash
# Portable Linux bundle: twi-scanner + shared libs (LibFT4222, readline, etc.)
# Requires LibFT4222 installed on the build machine (not mock). See README.

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-deploy"
DEPLOY_DIR="$PROJECT_DIR/deploy/dist"
APP_NAME="twi-scanner"

echo "=== Deploy $APP_NAME (CLI, Release + LibFT4222) ==="

cd "$PROJECT_DIR"
rm -rf "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DTWI_REQUIRE_FT4222=ON
cmake --build "$BUILD_DIR" -j"$(nproc)"

BIN="$BUILD_DIR/$APP_NAME"
if [[ ! -f "$BIN" ]]; then
    echo "ERROR: $BIN not found"
    exit 1
fi

rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR/bin" "$DEPLOY_DIR/lib"

cp "$BIN" "$DEPLOY_DIR/bin/"

ldd "$BIN" | awk '/=> \// {print $3}' | while read -r lib; do
    [[ -f "$lib" ]] && cp -n "$lib" "$DEPLOY_DIR/lib/" 2>/dev/null || true
done

for libpath in /usr/local/lib/libft4222.so* /usr/lib/*/libft4222.so*; do
    [[ -e "$libpath" ]] && cp -n "$libpath" "$DEPLOY_DIR/lib/" 2>/dev/null || true
done

cat > "$DEPLOY_DIR/run.sh" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:${LD_LIBRARY_PATH:-}"
exec "$SCRIPT_DIR/bin/twi-scanner" "$@"
EOF
chmod +x "$DEPLOY_DIR/run.sh"

ARCHIVE="$PROJECT_DIR/deploy/${APP_NAME}-portable-$(date +%Y%m%d).tar.gz"
tar -czf "$ARCHIVE" -C "$PROJECT_DIR/deploy" dist

echo "Bundle: $DEPLOY_DIR"
echo "Archive: $ARCHIVE"
echo "Run: cd $DEPLOY_DIR && ./run.sh"
