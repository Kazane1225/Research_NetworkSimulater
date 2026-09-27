FROM emscripten/emsdk:latest

WORKDIR /project

# Build SDL3 from source, with -pthread, once as part of the image so the container is fully
# self-contained. This is required because the app now uses a real background compute worker
# thread (src/compute_job.c), which needs every linked object -- including SDL3 itself -- to be
# built with pthread/atomics support; a prebuilt, non-pthread SDL3 (e.g. mounted in from the
# host via a stale SDL3_PREFIX) fails to link against pthread-enabled application code.
RUN apt-get update && apt-get install -y --no-install-recommends git cmake && \
    rm -rf /var/lib/apt/lists/* && \
    git clone --depth 1 https://github.com/libsdl-org/SDL.git /tmp/sdl3src && \
    cd /tmp/sdl3src && \
    emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release -DSDL_STATIC=ON -DSDL_SHARED=OFF \
      -DSDL_TESTS=OFF -DCMAKE_C_FLAGS=-pthread -DCMAKE_INSTALL_PREFIX=/sdl3prefix && \
    emmake cmake --build build -j"$(nproc)" && \
    emmake cmake --install build && \
    cd / && rm -rf /tmp/sdl3src

CMD mkdir -p website && \
    rm -f website/index.js website/index.data website/index.wasm && \
    emcc \
      src/main.c src/queue.c src/vector.c src/buffer.c src/shader.c \
      src/font.c src/text.c src/graphics.c src/history_tree.c src/entity.c \
      src/examples.c src/network.c src/auxdata.c src/stabilizing_algo.c \
      src/terminating_algo.c src/compute_job.c src/render.c src/events.c \
      -O2 \
      -pthread -s PTHREAD_POOL_SIZE=1 \
      -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 \
      -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=256mb \
      --preload-file assets/font.fon@/font.fon --preload-file assets/font.tga@/font.tga \
      -I /sdl3prefix/include \
      -L /sdl3prefix/lib \
      -lSDL3 \
      -Wall -Wextra -Wformat -Wshadow -Wundef -Wpointer-arith -Wcast-align -Wstrict-aliasing -Wwrite-strings \
      -o website/index.js
