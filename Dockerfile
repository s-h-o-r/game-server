FROM gcc:11.3 AS build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. -s compiler.libcxx=libstdc++11 --build=missing -s build_type=Release

COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

FROM ubuntu:22.04 AS run

RUN groupadd -r www && useradd -r -g www www
USER www

COPY --from=build /app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

ENTRYPOINT ["/app/game_server", "-c/app/data/config.json", "-w/app/static/", "--tick-period=50", "--randomize-spawn-points"]
