FROM emscripten/emsdk:latest

WORKDIR /project

CMD mkdir -p website && \
    rm -f website/index.js website/index.data website/index.wasm && \
    emcc \
      src/main.c src/queue.c src/vector.c src/buffer.c src/shader.c \
      src/font.c src/text.c src/graphics.c src/history_tree.c src/entity.c \
      src/examples.c src/network.c src/auxdata.c src/stabilizing_algo.c \
      src/terminating_algo.c src/render.c src/events.c \
      -O2 \
      -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 \
      -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=256mb \
      --preload-file assets/font.fon@/font.fon --preload-file assets/font.tga@/font.tga \
      -I /sdl3prefix/include \
      -L /sdl3prefix/lib \
      -lSDL3 \
      -Wall -Wextra -Wformat -Wshadow -Wundef -Wpointer-arith -Wcast-align -Wstrict-aliasing -Wwrite-strings \
      -o website/index.js
