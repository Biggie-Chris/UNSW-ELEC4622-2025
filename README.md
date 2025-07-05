# ELEC4622 - Image Signal Processing Course Repository

This repository contains all labs and projects for ELEC4622, a fourth-year UNSW Electrical Engineering course focused on image signal processing. All implementations are written in C/C++.

## 📚 Course Overview

ELEC4622 is an advanced course in image signal processing that covers fundamental concepts and practical implementations of various image processing techniques. The course emphasizes hands-on experience with real-world image processing algorithms.


## 🧪 Labs

### Lab 1: Basic Image I/O and Processing
- **Purpose**: Introduction to BMP file format and basic image manipulation
- **Features**:
  - BMP file reading and writing
  - Color channel separation (RGB)
  - Image brightness adjustment
  - Multi-frame processing support
- **Key Files**:
  - `lab1/src/main.cpp` - Main processing logic
  - `lab1/include/io_bmp.h` - BMP I/O interface
  - `lab1/include/memory_management.h` - Memory management utilities

### Lab 2: Image Filtering
- **Purpose**: Implementation of various image filtering techniques
- **Features**:
  - Mean averaging filter
  - Unsharp masking filter (H1)
  - Custom filters (H2, H3)
  - Boundary extension methods (zero padding, symmetric, zero-order hold)
- **Key Files**:
  - `lab2/src/filtering_main.cpp` - Main filtering implementation
  - `lab2/include/image_comps.h` - Image component definitions

### Lab 2 Optional: Advanced Filtering
- **Purpose**: Extended filtering examples and vectorized implementations
- **Features**:
  - Vertical filtering optimization
  - Vectorized filtering using SIMD instructions
  - Performance comparison between different approaches

## 📋 Project 1: Advanced Image Processing

### Task 1: Bilinear Interpolation
- **Implementation**: `project1/project1_task1/src/bi-linear_interpo_main.cpp`
- **Features**:
  - 3x image upscaling using bilinear interpolation
  - Support for both grayscale and RGB images
  - Optimized memory access patterns

### Task 2: Sinc Interpolation
- **Implementation**: `project1/project1_task2/src/sinc_interpolation_main.cpp`
- **Features**:
  - High-quality image upscaling using sinc interpolation
  - Hann window function for anti-aliasing
  - Configurable filter parameters

### Task 3: Image Differentiation
- **Implementation**: `project1/project1_task3/src/differentiation_main.cpp`
- **Features**:
  - Gradient computation using finite differences
  - Support for both horizontal and vertical differentiation
  - Configurable differentiation parameters

### Task 6: Difference of Gaussians (DoG)
- **Implementation**: `project1/project1_task6/src/DOG_main.cpp`
- **Features**:
  - Edge detection using DoG filter
  - Configurable sigma parameters (1-5 range)
  - Optional edge enhancement mode

## 🛠️ Technical Features

### Core Components
- **Aligned Image Components**: Optimized memory layout for SIMD operations
- **BMP I/O Library**: Efficient reading and writing of BMP files
- **Boundary Extension**: Multiple strategies for handling image boundaries
- **Vectorized Operations**: SIMD-optimized filtering using SSE instructions

### Performance Optimizations
- Memory-aligned data structures
- Vectorized filtering operations
- Efficient boundary extension algorithms
- Optimized convolution implementations

## 🚀 Getting Started

### Prerequisites
- Visual Studio 2019 or later (Windows)
- C++17 compatible compiler
- Media Interface Framework (included in tools/)

### Building the Projects
1. Open the solution file (`.sln`) in Visual Studio
2. Build the desired project (Debug/Release configuration)
3. Run the executable with appropriate command-line arguments

### Running Examples

#### Lab 1 - Basic Image Processing
```bash
# Brighten an image
lab1.exe barbara.bmp out.bmp 1
```

#### Lab 2 - Image Filtering
```bash
# Apply mean filter
lab2.exe barbara.bmp out_h1.bmp mean_avg
```

#### Project 1 - Advanced Processing
```bash
# Bilinear interpolation (3x upscaling)
project1_task1.exe barbara.bmp output.bmp

# Sinc interpolation
project1_task2.exe barbara.bmp output.bmp 10

# Image differentiation
project1_task3.exe barbara.bmp output.bmp 10 on

# Difference of Gaussians
project1_task6.exe barbara.bmp output.bmp 2.0 on
```

## 🎯 Key Algorithms Implemented

1. **Image Interpolation**
   - Bilinear interpolation for smooth upscaling
   - Sinc interpolation for high-quality reconstruction

2. **Image Filtering**
   - Mean filtering for noise reduction
   - Unsharp masking for edge enhancement
   - Custom convolution kernels

3. **Edge Detection**
   - Difference of Gaussians (DoG)
   - Gradient-based edge detection
   - Configurable scale parameters

4. **Optimization Techniques**
   - SIMD vectorization using SSE instructions
   - Memory-aligned data structures
   - Efficient boundary handling

## 📁 Data Files

The repository includes sample images for testing:
- `barbara.bmp` - Standard test image
- `pens_rgb.bmp` - Color test image
- Various output files demonstrating algorithm results

## 🛠️ Tools

### Media Interface Framework
Located in `tools/` directory, provides:
- `mi_viewer` - GUI for viewing BMP files
- `mi_pipe2` - Command-line image processing pipeline
- Various utility functions for image manipulation

---

**Note**: This repository contains educational implementations and should be used in accordance with UNSW academic integrity policies.


