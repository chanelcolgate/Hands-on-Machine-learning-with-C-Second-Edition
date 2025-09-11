#include <dlib/image_io.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_transforms.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	using namespace dlib;
	// dlib::array2d<dlib::rgb_pixel> img;
	array2d<rgb_pixel> img;

	// try {
	// 	if (argc > 1) {
	// 		if (fs::exists(argv[1])) {
	// 			const std::string file_path = argv[1];
	// 			// dlib::load_image(img, file_path);
	// 		} else {
	// 			std::cerr << "Invalid file path " << argv[1] << std::endl;
	// 			return 1;
	// 		}
	// 	} else {
	// 		long width = 512;
	// 		img.set_size(width, width);
	// 		assign_all_pixels(img, rgb_pixel(255, 255, 255));
	// 		fill_rect(
	// 				img,
	// 				rectangle(img.nc() / 4, img.nr() / 4, img.nc() / 2, img.nr() / 2),
	// 				rgb_pixel(0, 0, 0));
	// 	}

	// 	unsigned long key;
	// 	bool is_printable;
	// 	// show original image
	// 	image_window window(img, "Image");
	// 	window.get_next_keypress(key, is_printable);

	// 	// scale
	// 	// array2d<rgb_pixel> img2(img.nr() / 2, img.nc() / 2);
	// 	// there are also other interpolcation methods interpolate_quadratic() and
	// 	// interpolate_bilinear
	// 	// resize_image(img, img2, interpolate_nearest_neighbor());
	// 	// std::swap(img, img2);
	// 	// window.set_image(img);
	// 	// window.get_next_keypress(key, is_printable);

	// 	// resize_image(1.5, img); // default interpolate_bilinear
	// } catch (const std::exception& err) {
	// 	std::cerr << err.what();
	// }

	return 0;
}
