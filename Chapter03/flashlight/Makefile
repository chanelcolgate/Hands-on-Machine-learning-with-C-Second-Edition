.PHONY: all
.DELETE_ON_ERROR:

PLOTCPP_PATH := plotcpp/
ARRAYFIRE_PATH := arrayfire/build
LIBS_DIR 		 := /mnt/d/cpp/libs

all:
	cmake -DCMAKE_PREFIX_PATH=$(LIBS_DIR) -DFL_BUILD_ARRAYFIRE=ON -DArrayFire_DIR=$(LIBS_DIR)/$(ARRAYFIRE_PATH) -DPLOTCPP_PATH=$(LIBS_DIR)/$(PLOTCPP_PATH) -B ./build_tree
	cmake --build ./build_tree

run:
	./build_tree/grid_fl

.PHONY: clean
clean:
	rm -rf ./build_tree
