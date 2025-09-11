.PHONY: all
.DELETE_ON_ERROR:

PLOTCPP_PATH := plotcpp/
LIBS_DIR     := /mnt/d/cpp/libs

all:
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(LIBS_DIR) -DPLOTCPP_PATH=$(LIBS_DIR)/$(PLOTCPP_PATH) -B ./build_tree
	cmake --build ./build_tree

run:
	./build_tree/grid-dlib

.PHONY: clean

clean:
	rm -rf ./build_tree
