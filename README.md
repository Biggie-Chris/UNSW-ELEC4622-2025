# ELEC4622 - Image Signal Processing Course Repository

This repository contains all labs and projects for ELEC4622, a fourth-year UNSW Electrical Engineering course focused on image signal processing. All implementations are written in C/C++.

## 📚 Course Overview

ELEC4622 covers fundamental concepts and practical implementations of image processing techniques, emphasizing hands-on experience with real-world algorithms including filtering, frequency domain processing, motion estimation, and advanced resampling techniques.

## 🧪 Labs

### Lab 1: Basic Image I/O and Processing
- **Purpose**: Introduction to BMP file format and basic image manipulation
- **Features**: BMP I/O, RGB channel separation, brightness adjustment, multi-frame processing
- **Key Files**: `lab1/src/main.cpp`, `lab1/include/io_bmp.h`, `lab1/include/memory_management.h`

### Lab 2: Image Filtering
- **Purpose**: Implementation of various filtering techniques
- **Features**: Mean averaging, unsharp masking (H1), custom filters (H2, H3), boundary extension methods
- **Key Files**: `lab2/src/filtering_main.cpp`, `lab2/include/image_comps.h`

### Lab 2 Optional: Filtering Optimization
- **Purpose**: Performance-optimized filtering with SIMD
- **Features**: Vertical filtering optimization, vectorized SIMD implementations
- **Key Files**: `lab2_optional/filtering_example/`, `lab2_optional/vertical_filtering/`

### Lab 3: DFT and Frequency Domain Processing
- **Purpose**: Discrete Fourier Transform implementation and frequency filtering
- **Features**: DFT/IDFT algorithms, frequency domain filtering, spectral analysis
- **Key Files**: `lab3/dft_main.cpp`, `lab3/dft.cpp`, `lab3/dft.h`

### Lab 5: Motion Estimation
- **Purpose**: Block-based motion estimation and optical flow
- **Features**: Block matching algorithms, motion vector computation, temporal filtering
- **Key Files**: `lab5/motion_main.cpp`, `lab5/motion.h`

## 📋 Project 1: Resampling and Gradient Computation

### Task 1: Bilinear Interpolation
- **Implementation**: `project1/project1_task1/src/bi-linear_interpo_main.cpp`
- **Features**: 3x image upscaling, grayscale and RGB support

### Task 2: Sinc Interpolation
- **Implementation**: `project1/project1_task2/src/sinc_interpolation_main.cpp`
- **Features**: High-quality upscaling with Hann windowing

### Task 3: Image Differentiation
- **Implementation**: `project1/project1_task3/src/differentiation_main.cpp`
- **Features**: Gradient computation using finite differences

### Task 6: Derivative of Gaussians (DoG)
- **Implementation**: `project1/project1_task6/src/DOG_main.cpp`
- **Features**: Edge detection, configurable sigma (1-5 range), optional enhancement

## 📋 Project 2: Block Processing and Frequency Analysis

### Task 1: Block Extraction
- **Implementation**: `project2/project2_task1/src/extract_block_main.cpp`
- **Features**: Extract NxN blocks at specified coordinates (even-sized blocks only)

### Task 2: Hann Window Processing
- **Implementation**: `project2/project2_task2/src/hann_block_main.cpp`
- **Features**: Apply Hann window to blocks with proper visualization

### Task 3: DFT Block Processing
- **Implementation**: `project2/project2_task3/src/dft_main.cpp`
- **Features**: Block-based DFT for local frequency analysis

### Task 4: FFT Implementation
- **Implementation**: `project2/project2_task4/src/fft_main.cpp`
- **Features**: Fast Fourier Transform for efficient frequency analysis

### Task 5: RGB FFT Processing
- **Implementation**: `project2/project2_task5/src/rgb_fft_main.cpp`
- **Features**: FFT on RGB channels with spectral ring analysis

### Task 6: Texture Synthesis
- **Implementation**: `project2/project2_task6/src/texture_fft_main.cpp`
- **Features**: FFT-based texture analysis and synthesis

## 📋 Project 3: Global Motion Estimation and Compensation

### Task 1: Global Motion Estimation
- **Implementation**: `project3/project3_task1/src/task1_main.cpp`
- **Features**: Block matching for global motion vector estimation

### Task 2-5: Advanced Motion Analysis
- **Implementation**: `project3/project3_task2/src/task2_main.cpp` through `task5_main.cpp`
- **Features**: Motion compensation, temporal filtering, Harris corner detection

## 🛠️ Technical Features

### Core Components
- **Aligned Image Components**: SIMD-optimized memory layout (`aligned_image_comps.cpp`)
- **BMP I/O Library**: Efficient BMP file handling (`io_bmp.cpp`)
- **Boundary Extension**: Zero padding, symmetric extension, zero-order hold
- **Vectorized Operations**: SSE-optimized filtering
- **DFT/FFT Support**: Frequency domain processing capabilities

### Performance Optimizations
- Memory-aligned data structures
- Vectorized filtering operations
- Efficient boundary extension algorithms
- Optimized convolution implementations

## 🚀 Quick Start

### Prerequisites
- Visual Studio 2019+ (Windows)
- MSVC C++17 or higher
- Media Interface Framework (included in `tools/`)

### Running Examples

```bat
# Lab 1 - Brighten image
lab1.exe barbara.bmp out.bmp 1

# Lab 2 - Apply filtering
lab2.exe barbara.bmp out_h1.bmp mean_avg

# Project 1 - Bilinear interpolation
project1_task1.exe barbara.bmp output.bmp

# Project 1 - DoG edge detection
project1_task6.exe barbara.bmp output.bmp 2.0 on

# Project 2 - Block extraction
project2_task1.exe barbara.bmp output.bmp 16 100 100

# Project 3 - Global motion estimation
project3_task1.exe mb14.bmp mb15.bmp 10 10 20
```

## 📁 Data and Tools

- **Sample Images**: Located in respective `data/` directories
- **Batch Scripts**: `run_task*.bat` files for automated testing
- **Media Interface Framework**: Located in `tools/` directory
  - `mi_viewer` - GUI for viewing BMP files
  - `mi_pipe2` - Command-line processing pipeline
  - See `tools/README.md` for installation and usage

---

**Note**: This repository contains algorithm implementations only for UNSW educational purposes.
