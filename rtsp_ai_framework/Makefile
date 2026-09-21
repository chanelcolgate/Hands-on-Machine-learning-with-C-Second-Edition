.PHONY: all
.DELETE_ON_ERROR:

build:
	cmake -B ./build_tree
	cmake --build ./build_tree

run:
	./build_tree/rtsp_ai_framework

.PHONY: clean
	rm -rf ./build_tree
