CC 	 = gcc
CFLAGS   = -std=c11 -w
TARGET   = master
OBJFILES =
SOURCE = main.c
  
$(TARGET): $(OBJFILES)  
	$(CC) -o $(TARGET) $(SOURCE) $(CFLAGS) 

clean: 
	rm -f $(TARGET) *.o
