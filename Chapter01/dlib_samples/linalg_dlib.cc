#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/matrix.h>
#include <iostream>

int main() {
	// definitions
	{
		// compile time sized matrix
		dlib::matrix<double, 3, 1> y;
		// dynamically sized matrix
		dlib::matrix<double> m(3, 3);
		// later we can change size of this matrix;
		m.set_size(6, 6);
	}
	// initalizations
	{
		// comma operator
		dlib::matrix<double> m(3, 3);
		m = 1., 2., 3., 4., 5., 6., 7., 8., 9.;
		// std::cout << "Matrix from comman operator\n" << m << std::endl;

		// wrap array
		double data[] = {1, 2, 3, 4, 5, 6};
		auto m2 = dlib::mat(data, 2, 3); // create matrix with size 2x3
		std::cout << "Matrix from array\n" << m2 << std::endl;
		
		// Matrix elements can be accessed with () operator
		m(1, 2) = 300;
		// std::cout << "Matrix element updated\n" << m << std::endl;

		// Also you can initialize matrix with some predefined values
		auto a = dlib::identity_matrix<double>(3);
		std:: cout << "Identity matrix\n" << a << std::endl;

		auto b = dlib::ones_matrix<double>(3, 4);
		std::cout << "Ones matrix\n" << b << std::endl;

		auto c = dlib::randm(3, 4); //matrix with random values with size 3x4
		std::cout << "Random matrix\n" << c << std::endl;
	}
	// arithmetic operations
	{
		dlib::matrix<double> a(2, 2);
		a = 1, 1, 1, 1;
		dlib::matrix<double> b(2, 2);
		b = 2, 2, 2, 2;

		auto c = a + b;
		std::cout << "c = a + b\n" << c << std::endl;

		auto e = a * b; // real matrix multiplication
		std::cout << "e = a dot b\n" << e << std::endl;

		a += 5;
		std::cout << "a += 5\n" << a << std::endl;

		auto d = dlib::pointwise_multiply(a, b); // element wise multiplication
		std::cout << "d = a * b\n" << d << std::endl;

		auto t = dlib::trans(a); // transpose matrix
		std::cout << "transposed matrix a \n" << t << std::endl;
	}
	// partial access
	{
		dlib::matrix<float> m(4,4);
		m = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16;
		auto a1 = dlib::rowm(m,0); // take first row of matrix
		std::cout << "First row of matrix\n" << a1 << std::endl;

		dlib::matrix<float> a2(2,4);
		a2 = dlib::rowm(m,dlib::range(0,1));
		std::cout << "First two rows of matrix\n" << a2 << std::endl;

		auto a3 = dlib::colm(m,0);
		std::cout << "First column of matrix\n" << a3 << std::endl;

		dlib::matrix<float> sm(2,2);
		sm = dlib::subm(m,dlib::range(0,1),dlib::range(0,1)); // takes first two row
		std::cout << "Sub matrix\n" << sm << std::endl;

		// initialize part of the matrix
		dlib::set_subm(m,dlib::range(0,1),dlib::range(0,1)) = 7;
		std::cout << "Initalize part of matrix\n" << m << std::endl;

		// add a value to the part of the matrix
		dlib::set_subm(m,dlib::range(0,1),dlib::range(0,1)) += 7;
		std::cout << "Add a value to the part of the matrix\n" << m << std::endl;
	}
	// there are no implicit broadcasting in dlib
	{
		// we can simulate broadcasting with partial access
		dlib::matrix<float, 2, 1> v;
		v = 10, 10;
		dlib::matrix<float, 2, 3> m;
		m = 1, 2, 3, 4, 5, 6;
		for (int i = 0; i < m.nc(); i++) {
			dlib::set_colm(m, i) += v;
		}
		std::cout << "Matrix with updated columns \n" << m << std::endl;
	}
	return 0;
};
