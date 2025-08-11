# different compilers are required to link in soundtouch as a library
CC = gcc
CXX = g++

CFLAGS ?= -std=c17 -g\
	-D_DEFAULT_SOURCE\
	-Wall
CXXFLAGS ?= -std=c++17 -g -Wall -Wextra
LDFLAGS ?= -L/mingw64/lib -lncursesw -lSoundTouchDll -lportaudio -g
CPPFLAGS ?= -I/mingw64/include/ncursesw -I/mingw64/include/soundtouch -I/mingw64/include -g

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

.PHONY: main

vpath %.c handlers test
vpath %.cpp cpplib

handlers := wav.o
blocks := input.o loop.o main.o
adts := track.o queue.o
audio_helpers := audio.o audio_writer.o
extension_tests := test.o queue.o test_queue.o test_wav.o wav.o main.o test_input.o input.o test_audio.o audio.o audio_writer.o track.o test_track.o

cpp_wrappers := soundtouch_wrapper.o

main: $(handlers) $(blocks) $(adts) $(audio_helpers) $(cpp_wrappers)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests: CFLAGS += -DTEST_BUILD
tests: $(extension_tests) $(cpp_wrappers)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) -rf *.o main tests

format:
	find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
