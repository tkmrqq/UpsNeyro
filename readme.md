## Build dependencies

### Windows

1. vcpkg: https://github.com/microsoft/vcpkg
2. `vcpkg install ffmpeg:x64-windows`
3. При запуске cmake: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

### Linux

`sudo pacman -S ffmpeg` или `sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev`

## Updates (GitHub Releases)

В настройках приложения в разделе `Updates` задайте URL GitHub API последнего релиза, например:

`https://api.github.com/repos/<OWNER>/<REPO>/releases/latest`

Требуется HTTPS (OpenSSL) в сборке Qt на Windows.

В `tag_name` релиза используйте семантическую версию (`v0.2.0` или `0.2.0`), чтобы сравнение версий работало корректно.

## Unit tests (локально)

Полная конфигурация приложения тянет FFmpeg и Qt Quick; юнит-тесты собираются отдельным таргетом:

```bash
cmake -S . -B build -DUPSNEYRO_BUILD_TESTS=ON
cmake --build build --target test_projectmanager
ctest --test-dir build --output-on-failure
```

На Windows перед `ctest` добавьте в `PATH` каталог `bin` вашей установки Qt (иначе `test_projectmanager.exe` не найдёт `Qt6Core.dll`).

## Логи

Файл лога: каталог данных приложения, файл `upsneyro.log` (см. «Show log in file manager» в настройках). Включите «Verbose log file», чтобы в файл писались строки уровня DEBUG.

## CI

В GitHub Actions выполняется проверка синтаксиса `ai_engine/upscaler.py`. Сборка всего приложения на раннере без вашего FFmpeg/Qt не воспроизводится один в один с локальной средой.
