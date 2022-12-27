CXXFLAGS = -O2 -DNDEBUG -Ifcsim -I.

# Windows:
LDFLAGS = -lglfw3 -lgdi32 -lopengl32
TARGET  = main.exe

# Linux:
#LDFLAGS = -lGL -lglfw
#TARGET  = main
