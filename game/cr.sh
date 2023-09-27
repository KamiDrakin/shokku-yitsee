gcc -o game main.c -lraylib -lGL -lm -pthread -ldl -lrt -lX11 -DPLATFORM_DESKTOP -fsanitize=address && ./game
