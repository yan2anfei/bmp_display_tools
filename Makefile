SRC:=test_bmp.c bmp.c my_fb.c utils.c
TARGET:=test_bmp

$(TARGET):$(SRC:%.c=%.o)
	$(CC) -o $@ $^
%.o:%.c
	$(CC) -c -o $@ $<
clean:
	-@rm -rf *.o $(TARGET)
.PHONY:clean
