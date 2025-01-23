# Веб-игра "В поисках пропавших вещей"
Сервер веб-игры написано на языке C++ и представляет собой API для взаимодействия с моделью игры. Предполагается, что сервер может работать как локально, так и с использованием docker контейнера. Сервер обрабатывает запросы в многопоточном режиме и предназначет для одновременного подключения большого количества игроков. В игре присутсвуют несколько карт, которые также могуть быть модифицированы с помощью JSON файла. Сам код сервера написан для простой интеграции через REST API, добавлена возможность тестирования и отладки через служебные точки входа.

## Содержание

## Технологии
- [C++20](https://en.cppreference.com/w/cpp/20)
- [Boost](https://www.boost.org/)
- [Conan](https://docs.conan.io/1/index.html) v1.*
- [CMake](https://cmake.org/) v3.11+
- [Docker](https://www.docker.com/)

## Запуск сервера локально
Проект запускает просто через CMake, который позволяет развернуть приложение на различных системах.
1. Клонируйте репозиторий
```
git clone https://github.com/s-h-o-r/game-server.git
```
2. Перейдите в директорию проекта
```
cd game-server
```
3. Создайте папку build и запустите пакетный менеджер conan для загрузки необходимых файлов для сборки. Для запуска на Linux:
```
mkdir build && cd build
conan install .. -s compiler.libcxx=libstdc++11 --build=missing -s build_type=Release
```
4. Соберите проект с использованием CMake
```
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```
5. Сервер может быть запущен с различными параметрами. Вот все доступные параметры. Их также можно получить с помощью парамтра `-h` к исполняемому файлу:
```
Basic usage: game_server
  --tick-period <tick-period in ms> (optional)
  --config-file <game-config-json>
  --www-root <static-files-dir>
  --randomize-spawn-points (optional)
  --state-file <state-file-path> (optional)
  --save-state-period <tick-period in ms> (optional)
```
Обязательными параметрами являются `config-file` и `www-root`, без них сервер не запустится. Все остальные параметры являются необязательными и могут быть использованы опционально для более тонкой настройки сервера или запуска режима тестирования и отладки с ручным управлением обновления сервера.

## Запуск сервера с помощью Docker-контейнера
Сервер может работать с помощью Docker-контейнера. Для запуска контейнера установить Docker на свой сервер (все инструкции можно найти на [официальном сайте](https://www.docker.com/)). После установки Docker'а перейдем в папку с файлами нашего сервера и запустим следующие команды:
```
docker build -t my_http_server .
docker run --rm -p 80:8080 my_http_server
```

Не забудьте изменить в Dockerfile параметр `ENTRYPOINT`, который настраивает сервер через командную строку. Пример использования из поставляемого в проекте Dockerfile:
```
ENTRYPOINT ["/app/game_server", "-c/app/data/config.json", "-w/app/static/"]
```
Все возможные параметры указаны в разделе ["Запуск сервера локально"](#запуск-сервера-локально)

# Разработка
## Тестирование

## Ручное тестирование сервера

