.PHONY: all
.DELETE_ON_ERROR:

PLOTCPP_PATH 	:= plotcpp/
MLPACK_PATH 	:= mlpack/src
LIBS_DIR 			:= /mnt/d/cpp/libs

all:
	cmake -DCMAKE_PREFIX_PATH=$(LIBS_DIR) -DPLOTCPP_PATH=$(LIBS_DIR)/plotcpp -DMLPACK_INCLUDE_DIR=$(LIBS_DIR)/$(MLPACK_PATH) -B ./build_tree
	cmake --build ./build_tree

run:
	./build_tree/grid-mlpack

.PHONY: clean
clean:
	rm -rf ./build_tree
