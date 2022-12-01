CXXFLAGS = -O2 -DNDEBUG -I.

# Windows:
LDFLAGS = -lglfw3 -lgdi32 -lopengl32 -lws2_32
TARGET  = main.exe

# Linux:
#LDFLAGS = -lGL -lglfw
#TARGET  = main
