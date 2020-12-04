CC = gcc
CXX = g++

FILE_IO_CXX = fileio.cpp

AUDIO_FILES = $(FILE_IO_CXX) audiotag.cpp

FILE_IO_FILES = $(FILE_IO_CXX)

AUDIO_OBJ = $(AUDIO_FILES:.o=.c)
FILE_OBJ = $(FILE_IO_FILES:.o=.c)

all: audio

audio: $(AUDIO_OBJ)
	$(CXX) $(AUDIO_OBJ) -o audiotagger

%.o : %.c
	$(CC) -c $< -o $@

%.o : %.cpp
	$(CXX) -c $< -o $@

clean:
	-rm *.o