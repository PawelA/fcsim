CXXFLAGS = -O2 -DNDEBUG -Ifcsim -I.

# Windows:
LDFLAGS = -lglfw3 -lgdi32 -lopengl32
TARGET  = main.exe

# Linux:
#LDFLAGS = -lGL -lglfw
#TARGET  = main

# wasm:
#LD = wasm-ld
#LDFLAGS = --no-entry -export-all
#CC = clang
#CFLAGS = --target=wasm32 -nostdlib -I fcsim/stdlib -O2
#CXX = clang++
#CXXFLAGS = --target=wasm32 -nostdlib -fno-rtti -fno-exceptions -I fcsim/stdlib -O2
