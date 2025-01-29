# Веб-игра "В поисках пропавших вещей"
Сервер веб-игры написано на языке C++ и представляет собой API для взаимодействия с моделью игры. Предполагается, что сервер может работать как локально, так и с использованием docker контейнера. Сервер обрабатывает запросы в многопоточном режиме и предназначет для одновременного подключения большого количества игроков. В игре присутствуют несколько карт, которые также могут быть модифицированы с помощью JSON файла. Сам код сервера написан для простой интеграции через REST API, добавлена возможность тестирования и отладки через служебные точки входа.

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
5.Перед запуском сервера необходимо определить переменную среды окружения `GAME_DB_URL` для того, чтобы сервер смог сохранять таблицу лидеров в базу данных. Переменная среды должна быть представлена в формате URL и иметь следующий вид:
```
postgres://user:pass@host:port/dbname
```
Как установить переменную среды окружения на вашей системе вы можете посмотреть по следующим ссылкам: [Linux](https://wiki.merionet.ru/articles/peremennye-okruzheniya-v-linux-kak-posmotret-ustanovit-i-sbrosit) (подойдет для MacOS с оболочкой bash или zsh) и [Windows](https://ru.stackoverflow.com/questions/229/%D0%9A%D0%B0%D0%BA-%D1%83%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%B8%D1%82%D1%8C-%D0%BF%D0%B5%D1%80%D0%B5%D0%BC%D0%B5%D0%BD%D0%BD%D1%83%D1%8E-%D0%BE%D0%BA%D1%80%D1%83%D0%B6%D0%B5%D0%BD%D0%B8%D1%8F-%D0%B2-windows).

6. Сервер может быть запущен с различными параметрами. Вот все доступные параметры. Их также можно получить с помощью параметра `-h` к исполняемому файлу:
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
Сервер может работать с помощью Docker-контейнера. Для запуска контейнера установить Docker на свой сервер (все инструкции можно найти на [официальном сайте](https://www.docker.com/)). В данной инструкции мы пойдем по легкому пути и установим базу данных локально. 

Заходим в БД и создаем таблицу с названием `leaderboard_game_db` (название может быть любым, главное не забудьте дальше его поменять, если выбрали свое название).

После установки Docker'а перейдем в папку с файлами нашего сервера и запустим следующие команды, заменив `<user>` `<pass>` на данные для входа в локальную БД:
```
docker build -t my_http_server .
docker run --rm -p 80:8080 -e GAME_DB_URL=postgres://<user>:<pass>@host.docker.internal:5432/leaderboard_game_db my_http_server
```

Не забудьте изменить в Dockerfile параметр `ENTRYPOINT`, который настраивает сервер через командную строку. Пример использования из поставляемого в проекте Dockerfile:
```
ENTRYPOINT ["/app/game_server", "-c/app/data/config.json", "-w/app/static/", "--tick-period50", "--randomize-spawn-points"]
```
С данными настройками сервер запускает стандартный config файл с тремя картами, устанавливает частоту обновления сервера на 50 миллисекунд и случайную точку появления игрока на карте.

Все возможные параметры указаны в разделе ["Запуск сервера локально"](#запуск-сервера-локально)

