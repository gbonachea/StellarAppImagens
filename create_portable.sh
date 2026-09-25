#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PORTABLE_DIR="$SCRIPT_DIR/StellarAppImagens-portable"
BINARY_NAME="StellarAppImagens"

echo "=== Stellar AppImagens - Empaquetado Portable ==="
echo ""

# 1. Build Release
echo "[1/5] Compilando en modo Release..."
cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -1
cmake --build "$BUILD_DIR" -j"$(nproc)" 2>&1 | tail -1

BINARY_PATH="$BUILD_DIR/$BINARY_NAME"
if [ ! -f "$BINARY_PATH" ]; then
    echo "ERROR: No se encontró el binario compilado."
    exit 1
fi

# 2. Crear estructura portable
echo "[2/5] Creando estructura portable..."
rm -rf "$PORTABLE_DIR"
mkdir -p "$PORTABLE_DIR/lib"
mkdir -p "$PORTABLE_DIR/platforms"
mkdir -p "$PORTABLE_DIR/styles"
mkdir -p "$PORTABLE_DIR/imageformats"
mkdir -p "$PORTABLE_DIR/iconengines"
mkdir -p "$PORTABLE_DIR/xcbglintegrations"
mkdir -p "$PORTABLE_DIR/tls"

cp "$BINARY_PATH" "$PORTABLE_DIR/"
chmod +x "$PORTABLE_DIR/$BINARY_NAME"

# 3. Copiar librerías del sistema
echo "[3/5] Copiando librerías del sistema..."
ldd "$BINARY_PATH" | grep -oP '=> \K[^ ]+' | while read -r lib; do
    if [ -f "$lib" ] && [ ! -L "$lib" ]; then
        cp -L "$lib" "$PORTABLE_DIR/lib/" 2>/dev/null || true
    elif [ -L "$lib" ]; then
        target="$(readlink -f "$lib" 2>/dev/null)"
        if [ -f "$target" ]; then
            cp -L "$target" "$PORTABLE_DIR/lib/" 2>/dev/null || true
            libname="$(basename "$lib")"
            targetname="$(basename "$target")"
            if [ "$libname" != "$targetname" ]; then
                ln -sf "$targetname" "$PORTABLE_DIR/lib/$libname" 2>/dev/null || true
            fi
        fi
    fi
done

# 4. Copiar plugins Qt
echo "[4/5] Copiando plugins Qt..."
QT_PLUGIN_PATH="$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || qmake -query QT_INSTALL_PLUGINS 2>/dev/null || echo "")"

if [ -z "$QT_PLUGIN_PATH" ] || [ ! -d "$QT_PLUGIN_PATH" ]; then
    for candidate in \
        /usr/lib/x86_64-linux-gnu/qt6/plugins \
        /usr/lib/qt6/plugins \
        /usr/lib64/qt6/plugins \
        /usr/local/lib/qt6/plugins \
        "$HOME/.local/lib/qt6/plugins"; do
        if [ -d "$candidate" ]; then
            QT_PLUGIN_PATH="$candidate"
            break
        fi
    done
fi

if [ -z "$QT_PLUGIN_PATH" ] || [ ! -d "$QT_PLUGIN_PATH" ]; then
    echo "  ADVERTENCIA: No se encontró QT_INSTALL_PLUGINS. Los plugins se copiarán manualmente."
    QT_PLUGIN_PATH=""
fi

if [ -n "$QT_PLUGIN_PATH" ] && [ -d "$QT_PLUGIN_PATH" ]; then
    echo "  Plugins Qt: $QT_PLUGIN_PATH"

    # platforms (obligatorio)
    for f in "$QT_PLUGIN_PATH/platforms/"*.so; do
        [ -f "$f" ] && cp "$f" "$PORTABLE_DIR/platforms/"
    done

    # styles
    for f in "$QT_PLUGIN_PATH/styles/"*.so; do
        [ -f "$f" ] && cp "$f" "$PORTABLE_DIR/styles/"
    done

    # imageformats
    for f in "$QT_PLUGIN_PATH/imageformats/"*.so; do
        [ -f "$f" ] && cp "$f" "$PORTABLE_DIR/imageformats/"
    done

    # iconengines
    for f in "$QT_PLUGIN_PATH/iconengines/"*.so; do
        [ -f "$f" ] && cp "$f" "$PORTABLE_DIR/iconengines/"
    done

    # xcbglintegrations
    if [ -d "$QT_PLUGIN_PATH/xcbglintegrations" ]; then
        for f in "$QT_PLUGIN_PATH/xcbglintegrations/"*.so; do
            [ -f "$f" ] && cp "$f" "$PORTABLE_DIR/xcbglintegrations/"
        done
    fi

    # tls
    if [ -d "$QT_PLUGIN_PATH/tls" ]; then
        for f in "$QT_PLUGIN_PATH/tls/"*.so; do
            [ -f "$f" ] && cp "$f" "$PORTABLE_DIR/tls/"
        done
    fi

    # Copiar dependencias de los plugins también
    for plugin_dir in platforms styles imageformats iconengines xcbglintegrations tls; do
        for libfile in "$PORTABLE_DIR/$plugin_dir/"*.so; do
            [ -f "$libfile" ] || continue
            ldd "$libfile" 2>/dev/null | grep -oP '=> \K[^ ]+' | while read -r dep; do
                if [ -f "$dep" ] && [ ! -L "$dep" ]; then
                    libname="$(basename "$dep")"
                    if [ ! -f "$PORTABLE_DIR/lib/$libname" ]; then
                        cp -L "$dep" "$PORTABLE_DIR/lib/" 2>/dev/null || true
                    fi
                elif [ -L "$dep" ]; then
                    target="$(readlink -f "$dep" 2>/dev/null)"
                    libname="$(basename "$dep")"
                    targetname="$(basename "$target")"
                    if [ -f "$target" ] && [ ! -f "$PORTABLE_DIR/lib/$libname" ]; then
                        cp -L "$target" "$PORTABLE_DIR/lib/" 2>/dev/null || true
                        if [ "$libname" != "$targetname" ]; then
                            ln -sf "$targetname" "$PORTABLE_DIR/lib/$libname" 2>/dev/null || true
                        fi
                    fi
                fi
            done
        done
    done
else
    # Copiar plugins manualmente si qmake no disponible
    for candidate in \
        /usr/lib/x86_64-linux-gnu/qt6/plugins \
        /usr/lib/qt6/plugins; do
        if [ -d "$candidate/platforms" ]; then
            cp "$candidate/platforms/"*.so "$PORTABLE_DIR/platforms/" 2>/dev/null || true
            [ -d "$candidate/styles" ] && cp "$candidate/styles/"*.so "$PORTABLE_DIR/styles/" 2>/dev/null || true
            [ -d "$candidate/imageformats" ] && cp "$candidate/imageformats/"*.so "$PORTABLE_DIR/imageformats/" 2>/dev/null || true
            [ -d "$candidate/iconengines" ] && cp "$candidate/iconengines/"*.so "$PORTABLE_DIR/iconengines/" 2>/dev/null || true
            break
        fi
    done
fi

# 5. Crear script launcher
echo "[5/5] Creando script de inicio..."
cat > "$PORTABLE_DIR/run.sh" << 'RUNEOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$DIR"
export QT_QPA_PLATFORM_PLUGIN_PATH="$DIR/platforms"
export XDG_DATA_DIRS="$DIR:$XDG_DATA_DIRS"
exec "$DIR/StellarAppImagens" "$@"
RUNEOF
chmod +x "$PORTABLE_DIR/run.sh"

# Resumen
LIB_COUNT=$(find "$PORTABLE_DIR/lib" -maxdepth 1 -type f -name "lib*" 2>/dev/null | wc -l)
PLUGIN_COUNT=$(find "$PORTABLE_DIR" -maxdepth 2 -name "*.so" ! -path "*/lib/*" 2>/dev/null | wc -l)
TOTAL_SIZE=$(du -sh "$PORTABLE_DIR" | cut -f1)

echo ""
echo "=== Empaquetado completado ==="
echo "  Ubicación:  $PORTABLE_DIR"
echo "  Librerías:  $LIB_COUNT"
echo "  Plugins:    $PLUGIN_COUNT"
echo "  Tamaño:     $TOTAL_SIZE"
echo ""
echo "Para ejecutar:  ./StellarAppImagens-portable/run.sh"
echo "  o directamente: ./StellarAppImagens-portable/$BINARY_NAME"
echo "    (con LD_LIBRARY_PATH apuntando a lib/)"
