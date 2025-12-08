#!/bin/bash
# deploy_qt6_full.sh - полный деплой Qt6/QML приложения

PROJECT_DIR="$HOME/WorkDir/twi-scanner"
BUILD_DIR="$PROJECT_DIR/build-for-deploy"
DEPLOY_DIR="$PROJECT_DIR/deploy"
APP_NAME="twi-scanner"
QT_DIR="/usr/lib/x86_64-linux-gnu/qt6"  # Путь к Qt6

echo "=== Полный деплой Qt6/QML приложения $APP_NAME ==="

# 1. Сборка проекта
cd "$PROJECT_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Сборка проекта..."
cmake ..
make -j$(nproc)

if [ ! -f "$APP_NAME" ]; then
    echo "ОШИБКА: Исполняемый файл $APP_NAME не найден!"
    exit 1
fi

echo "Сборка завершена успешно!"

# 2. Создать структуру дистрибутива
echo "Создание структуры дистрибутива..."
DIST_DIR="$DEPLOY_DIR/dist"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/lib"
mkdir -p "$DIST_DIR/qml"
mkdir -p "$DIST_DIR/plugins"
mkdir -p "$DIST_DIR/translations"

# 3. Копировать исполняемый файл
cp "$APP_NAME" "$DIST_DIR/bin/"

# 4. Копировать библиотеки
echo "Копирование библиотек..."

# Основные библиотеки приложения
ldd "$BUILD_DIR/$APP_NAME" | grep "=> /" | awk '{print $3}' | xargs -I '{}' cp -v '{}' "$DIST_DIR/lib/" 2>/dev/null

# Qt6 библиотеки
QT_LIBS=(
    "libQt6Core.so"
    "libQt6Gui.so"
    "libQt6Qml.so"
    "libQt6Quick.so"
    "libQt6Widgets.so"
    "libQt6Network.so"
    "libQt6OpenGL.so"
)

for lib in "${QT_LIBS[@]}"; do
    find /usr/lib -name "$lib*" -exec cp -v {} "$DIST_DIR/lib/" \; 2>/dev/null
done

# 5. Копировать QML модули
echo "Копирование QML модулей..."
QML_MODULES=(
    "QtQml"
    "QtQuick"
    "QtQuick/Controls"
    "QtQuick/Layouts"
    "QtQuick/Templates"
    "QtQuick/Window"
)

for module in "${QML_MODULES[@]}"; do
    MODULE_DIR="/usr/lib/x86_64-linux-gnu/qt6/qml/$module"
    if [ -d "$MODULE_DIR" ]; then
        mkdir -p "$DIST_DIR/qml/$module"
        cp -r "$MODULE_DIR"/* "$DIST_DIR/qml/$module/" 2>/dev/null
    fi
done

# 6. Копировать плагины
echo "Копирование плагинов..."
PLUGINS=(
    "platforms"
    "imageformats"
    "platforminputcontexts"
    "xcbglintegrations"
)

for plugin in "${PLUGINS[@]}"; do
    PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/qt6/plugins/$plugin"
    if [ -d "$PLUGIN_DIR" ]; then
        mkdir -p "$DIST_DIR/plugins/$plugin"
        cp -r "$PLUGIN_DIR"/* "$DIST_DIR/plugins/$plugin/" 2>/dev/null
    fi
done

# 7. Копировать специфичные библиотеки проекта (libft4222)
if [ -f "/usr/local/lib/libft4222.so" ]; then
    cp "/usr/local/lib/libft4222.so" "$DIST_DIR/lib/"
    echo "Скопирована библиотека libft4222.so"
fi

# 8. Создать скрипт запуска
echo "Создание скрипта запуска..."
cat > "$DIST_DIR/run.sh" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
export QML2_IMPORT_PATH="$SCRIPT_DIR/qml"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"
"$SCRIPT_DIR/bin/twi-scanner" "$@"
EOF

chmod +x "$DIST_DIR/run.sh"

# 9. Создать .desktop файл
cat > "$DIST_DIR/$APP_NAME.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=TWI Scanner
Comment=I2C/TWI Scanner Tool
Exec=sh -c 'cd "\$(dirname "\$0")" && ./run.sh'
Icon=
Categories=Utility;Development;Electronics;
Terminal=false
EOF

# 10. Создать README
cat > "$DIST_DIR/README.txt" << EOF
TWI Scanner - портативная версия

Запуск:
1. Откройте терминал в этой папке
2. Выполните: ./run.sh

Или дважды кликните на run.sh в файловом менеджере

Требования:
- Linux с графической оболочкой
- OpenGL поддержка

Содержимое:
- bin/     - исполняемый файл
- lib/     - библиотеки
- qml/     - QML модули Qt6
- plugins/ - плагины Qt6
- run.sh   - скрипт запуска

Версия: $(date +%Y-%m-%d)
EOF

# 11. Создать архив
echo "Создание архива..."
cd "$DEPLOY_DIR"
tar -czf "twi-scanner-portable-$(date +%Y%m%d).tar.gz" -C "$DEPLOY_DIR" dist/

echo ""
echo "=== ДЕПЛОЙ ЗАВЕРШЕН ==="
echo "Портативная версия создана в: $DIST_DIR"
echo "Архив создан: $DEPLOY_DIR/twi-scanner-portable-$(date +%Y%m%d).tar.gz"
echo ""
echo "Для тестирования:"
echo "cd $DIST_DIR && ./run.sh"
echo ""
echo "Для установки на другую систему:"
echo "1. Распаковать архив"
echo "2. Запустить ./run.sh"