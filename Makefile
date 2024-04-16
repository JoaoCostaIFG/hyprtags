all:
	$(CXX) -shared -fPIC --no-gnu-unique main.cpp -o hyprtags.so -g `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++2b -O2

clean:
	rm ./hyprtags.so
