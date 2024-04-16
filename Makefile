NAME=hyprtags

SRC=$(wildcard src/*.cpp)
INC=$(wildcard include/*.hpp)

$(NAME).so: $(SRC) $(INC)
	$(CXX) -shared -fPIC --no-gnu-unique $(SRC) -o $(NAME).so -g `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++2b -O2 -Iinclude

clean:
	rm ./$(NAME).so
