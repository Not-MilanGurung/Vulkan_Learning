CFLAGS = -std=c++23 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp
	g++ $(CFLAGS) -g -UNDEBUG -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: test clean

release: 
        g++ $(CFLAGS) -DNDEBUG -o VulkanTestR main.cpp $(LDFLAGS)

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest

rel: VulkanTest
	./VulkanTestR
