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

### Lab 2 Optional: Filtering with Performance Optimization
- **Purpose**: Extended filtering examples and vectorized implementations
- **Features**:
  - Vertical filtering optimization
  - Vectorized filtering using SIMD instructions
  - Performance comparison between different approaches
- **Key Files**:
  - `lab2_optional/filtering_example/filtering_main.cpp` - Basic filtering example
  - `lab2_optional/vertical_filtering/filtering_main.cpp` - Optimized vertical filtering
  - `lab2_optional/vertical_filtering/vector_filter.cpp` - SIMD implementations

### Lab 3: DFT and Frequency Domain Processing
- **Purpose**: Discrete Fourier Transform implementation and frequency domain filtering
- **Features**:
  - DFT/IDFT implementation for frequency analysis
  - Frequency domain filtering techniques
  - Spectral analysis and visualization
- **Key Files**:
  - `lab3/dft_main.cpp` - Main DFT processing logic
  - `lab3/dft.cpp` - DFT algorithm implementation
  - `lab3/dft.h` - DFT function declarations
  - `lab3/image_comps.h` - Image component utilities

## 📋 Project 1: Resampling and Gradient Computation

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

### Task 6: Derivative of Gaussians (DoG)
- **Implementation**: `project1/project1_task6/src/DOG_main.cpp`
- **Features**:
  - Edge detection using DoG filter
  - Configurable sigma parameters (1-5 floating point range)
  - Optional edge enhancement mode
  - RGB output for visualization

## 📋 Project 2: Block Processing and Frequency Analysis

### Task 1: Block Extraction
- **Implementation**: `project2/project2_task1/src/extract_block_main.cpp`
- **Features**:
  - Extract NxN blocks from images at specified coordinates
  - Support for even-sized blocks only
  - Efficient memory management for block processing

### Task 2: Hann Window Block Processing
- **Implementation**: `project2/project2_task2/src/hann_block_main.cpp`
- **Features**:
  - Apply Hann window function to extracted blocks
  - Block-based signal processing
  - Visualization with proper offset for negative values

### Task 3: DFT Block Processing
- **Implementation**: `project2/project2_task3/src/dft_main.cpp`
- **Features**:
  - Discrete Fourier Transform on image blocks
  - Frequency domain analysis of local image features
  - Block-based spectral processing

### Task 4: FFT Implementation
- **Implementation**: `project2/project2_task4/src/fft_main.cpp`
- **Features**:
  - Fast Fourier Transform for efficient frequency analysis
  - Optimized processing for large image blocks
  - Real-time frequency domain operations

### Task 5: RGB FFT Processing
- **Implementation**: `project2/project2_task5/src/rgb_fft_main.cpp`
- **Features**:
  - FFT processing on RGB color channels
  - Color-aware frequency domain filtering
  - Multi-channel spectral analysis

### Task 6: Texture Synthesis using FFT
- **Implementation**: `project2/project2_task6/src/texture_fft_main.cpp`
- **Features**:
  - FFT-based texture analysis and synthesis
  - Advanced texture generation algorithms
  - Pattern recognition and reconstruction

## 🛠️ Technical Features

### Core Components
- **Aligned Image Components**: Optimized memory layout for SIMD operations
  - `project1/src/aligned_image_comps.cpp` - Project 1 aligned components
  - `project2/src/aligned_image_comps.cpp` - Project 2 enhanced components
- **BMP I/O Library**: Efficient reading and writing of BMP files
  - `project1/src/io_bmp.cpp` - Core BMP I/O functionality
  - `project2/src/io_bmp.cpp` - Enhanced BMP I/O with additional features
- **Boundary Extension**: Multiple strategies for handling image boundaries
- **Vectorized Operations**: SIMD-optimized filtering using SSE instructions
- **DFT/FFT Support**: Frequency domain processing capabilities
  - `lab3/dft.cpp` - Basic DFT implementation
  - `project2/src/dft.cpp` - Advanced DFT for block processing

### Performance Optimizations
- Memory-aligned data structures
- Vectorized filtering operations
- Efficient boundary extension algorithms
- Optimized convolution implementations

## 🚀 Getting Started

### Prerequisites
- Visual Studio 2019 or later (Windows)
- MSVC C++17 or higher
- Media Interface Framework (included in tools/)

### Building the Projects
1. Open the solution file (`.sln`) in Visual Studio
2. Build the desired project (Debug/Release configuration)
3. Run the executable with appropriate command-line arguments

### Running Examples

#### Lab 1 - Basic Image Processing
```bat
# Brighten an image
lab1.exe barbara.bmp out.bmp 1
```

#### Lab 2 - Image Filtering
```bat
# Apply mean filter
lab2.exe barbara.bmp out_h1.bmp mean_avg
```

#### Lab 3 - DFT Processing
```bat
# Apply DFT to an image
lab3.exe bike_mono.bmp out.bmp
```

#### Project 1 - Advanced Processing
```bat
# Bilinear interpolation (3x upscaling)
project1_task1.exe barbara.bmp output.bmp

# Sinc interpolation
project1_task2.exe barbara.bmp output.bmp 10

# Image differentiation
project1_task3.exe barbara.bmp output.bmp 10 on

# Derivative of Gaussians
project1_task6.exe barbara.bmp output.bmp 2.0 on
```

#### Project 2 - Block Processing and FFT
```bat
# Extract NxN block at coordinates (p1, p2)
project2_task1.exe barbara.bmp output.bmp 16 100 100

# Apply Hann window to extracted block
project2_task2.exe barbara.bmp output.bmp 16 100 100

# DFT on image blocks
project2_task3.exe bike_mono.bmp output.bmp 16 100 100

# FFT processing
project2_task4.exe bike_mono.bmp output.bmp 16 100 100

# RGB FFT processing
project2_task5.exe bike_mono.bmp output.bmp 16 100 100

# Texture synthesis using FFT
project2_task6.exe sanity.bmp output.bmp 32 100 100

```

## 📁 Data Files

The repository includes sample images for testing:
- **Lab 1 Data**: `lab1/data/`
  - `barbara1.bmp`, `barbara2.bmp`, `barbara3.bmp` - Standard test images
- **Lab 2 Data**: `lab2/data/`
  - `barbara1.bmp` - Input test image
  - `pens_rgb.bmp` - Color test image
- **Lab 3 Data**: `lab3/data/`
  - `bike_mono.bmp`, `pens_mono.bmp` - Monochrome test images
  - `sanity.bmp` - Verification image
- **Project Data**: Sample input and output files for all project tasks

## 🛠️ Tools

### Media Interface Framework
Located in `tools/` directory, provides:
- `mi_viewer` - GUI for viewing BMP files
- `mi_pipe2` - Command-line image processing pipeline
- Various utility functions for image manipulation

See `tools/README.md` for detailed installation instructions and more examples.


---

**Note**: This repository contains educational implementations and should be used in accordance with UNSW academic integrity policies.


