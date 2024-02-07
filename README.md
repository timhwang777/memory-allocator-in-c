# Memory Allocator in C

![c-badge](https://img.shields.io/badge/Solutions-blue?logo=C
)

## Table of Contents
- [About the Project](#about-the-project)
- [Getting Started](#getting-started)
  - [Project Overview](#project-overview)
  - [Prerequisites](#prerequisites)
- [Environment Setup](#environment-setup)
- [Build and Run with Make](#build-and-run-with-make)
- [Author](#author)

## About the Project
This project implements a custom memory management system in C, designed to provide various memory allocation strategies such as best fit, worst fit, first fit, next fit, and buddy system. It includes functions for initializing the memory region, allocating and freeing memory, and utilities for debugging memory state.

## Getting Started

### Project Overview
The system uses a linked list to manage memory blocks within a pre-allocated memory region. Each block contains metadata for managing the allocation and deallocation processes. The allocation algorithms can be selected during the initialization phase, supporting flexible memory management strategies tailored to specific application needs.

### Prerequisites
- A Linux environment or a similar system with support for POSIX APIs.
- GCC compiler for building the project.
- Basic understanding of memory management concepts and C programming.

## Environment Setup
1. Clone the repository to your local machine.
2. Ensure you have GCC installed by running `gcc --version`. Install GCC if it's not installed.
3. Navigate to the project directory where the source code is located.
4. To compile the program, use the following commands:
```bash
gcc -o umem umem.c
```

## Build and Run with Make
1. To compile the project, run `make` in the terminal within the project directory. This will create the executable named `main` as specified in the Makefile.
2. To execute the program, run `./main` in the terminal. This command will run the compiled program.

## Author
Timothy Hwang