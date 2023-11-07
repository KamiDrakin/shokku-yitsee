make:
	gcc -o game main.c -lraylib -lGL -lm -pthread -ldl -lrt -lX11 -DPLATFORM_DESKTOP

sanitized:
	gcc -o game main.c -lraylib -lGL -lm -pthread -ldl -lrt -lX11 -DPLATFORM_DESKTOP -fsanitize=address

win:
	gcc -o game main.c -lraylib -lGL -lm -DPLATFORM_DESKTOP	

run:
	./game
