rm web/*
emcc -o web/game.html main.c -Os -Wall ./libraylib.a -I. -I../../raylib/src/ -L. -L../../raylib/src/ -s USE_GLFW=3 --shell-file ./shell.html -s TOTAL_MEMORY=67108864 -DPLATFORM_WEB --preload-file ./res
mv web/game.html web/index.html
zip web/game.zip web/*
