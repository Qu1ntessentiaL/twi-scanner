# twi-scanner

Консольный CLI для работы с FT4222 (I2C / SPI / GPIO) через [LibFT4222](https://ftdichip.com/products/ft4222h/).

## Зависимости

- CMake ≥ 3.15, компилятор с C++17
- LibFT4222 + D2XX (см. ниже)
- `libreadline-dev` (опционально, история команд)

```bash
sudo apt install cmake ninja-build libreadline-dev
```

## Установка LibFT4222

1. Откройте https://ftdichip.com/products/ft4222h/ → вкладка **Downloads**
2. Скачайте **LibFT4222 Linux Library**
3. Распакуйте архив и выполните `sudo ./install4222.sh`
4. Заголовки попадут в `/usr/local/include`, библиотека — в `/usr/local/lib`

### Права доступа (udev)

Без root устройство может не открываться. Пример правила:

```
# /etc/udev/rules.d/99-ft4222.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="0403", MODE="0666"
```

После добавления: `sudo udevadm control --reload && sudo udevadm trigger`

## Сборка

С LibFT4222 (рабочая станция с устройством):

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Без LibFT4222 (CI, удалённый сервер) — mock-режим, без доступа к железу:

```bash
cmake -S . -B build -G Ninja -DTWI_MOCK_FT4222=ON
cmake --build build
```

Если библиотека не установлена, CMake автоматически включает mock (можно запретить: `-DTWI_REQUIRE_FT4222=ON`).

```bash
ctest --test-dir build   # unit-тесты
./build/twi-scanner --version
./build/twi-scanner -c devices   # в mock — один тестовый «MOCK0001»
```

## Использование

Интерактивный режим:

```bash
./build/twi-scanner
```

Пакетный режим:

```bash
./build/twi-scanner -c devices
./build/twi-scanner -c "connect 0" -c "i2c_init 400" -c "i2c_scan"
```

Подключение по серийному номеру (в т.ч. если номер состоит только из цифр):

```text
connect --serial A1B2C3
connect s:A1B2C3
```

### Основные команды

| Команда | Описание |
|---------|----------|
| `devices` | Список FT4222 |
| `connect <index>` | Подключение по индексу FTDI |
| `connect --serial <sn>` | Подключение по серийному номеру |
| `disconnect` | Отключение |
| `status` | Статус подключения |
| `i2c_init [100\|400\|1000]` | Инициализация I2C |
| `i2c_scan [start] [end]` | Сканирование шины |
| `i2c_send / i2c_recv` | Запись / чтение |
| `spi_init / spi_send / spi_recv / spi_xfer` | SPI |
| `gpio_init / gpio_read / gpio_write` | GPIO |
| `help [cmd]` | Справка |

## Деплой (portable bundle)

Собирается на машине **с установленной LibFT4222** (не mock). Скрипт кладёт бинарник, зависимости `ldd` и `libft4222.so` в `deploy/dist/`, плюс `run.sh` и `.tar.gz`.

```bash
chmod +x deploy/deploy.sh
./deploy/deploy.sh
./deploy/dist/run.sh -c devices
```

На целевой системе нужен только распакованный архив и USB-доступ к FT4222 (udev).

## Структура

- `src/cli/` — CLI, роутер команд, парсер чисел
- `src/ft4222/` — обёртка над LibFT4222
- `tests/` — unit-тесты (парсер, роутер)
