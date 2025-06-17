#pragma once
#include "io_bmp.h"

class io_comp {
public:
	int width;
	int height;
	int stride; // sample num for each row

	// 1. buf points to the top left of image component, we can use negatie indices to access samples
	// outside the original image boundaries
	// 2. handle points to the start of alloacted memory block (extened version)
	int* buf;
	int* handle;

public:
	void allocate_with_border(int border = 16) { // 16 by default
		stride = width + border * 2;
		int true_height = height + border * 2;

		handle = new int[stride * true_height];
		buf = handle + stride * border + border; // buf is a pointer shifted from handle
		// no need to delete
	}

	io_comp() : width(0), height(0), stride(0), buf(nullptr), handle(nullptr) {}
	~io_comp() { delete[] handle; }
};

// image contains several color components
class io_image {
public:
	int num_components;
	io_comp* comps;

	io_image() : num_components(0), comps(nullptr) {}
	~io_image() { delete[] comps; }
};

