all:
	# cmake -B ./build_tree -DCSV_LIB_PATH=/mnt/d/cpp/Hands-on-Machine-learning-with-C-Second-Edition/Chapter02/data -DFL_BUILD_ARRAYFIRE=ON -DArrayFire_DIR=/mnt/d/cpp/libs/arrayfire/build
	cmake --build ./build_tree
run:
	./build_tree/csv_fl /mnt/d/cpp/Hands-on-Machine-learning-with-C-Second-Edition/Chapter02/data/iris.data
clean:
	rm -rf ./build_tree
