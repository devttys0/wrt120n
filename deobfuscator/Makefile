TARGET=wrt120n

$(TARGET): crc.o
	$(CC) -Wall $(TARGET).c *.o -o $(TARGET)

crc.o:
	$(CC) -Wall crc.c -c

clean:
	rm -rf $(TARGET) *.o
