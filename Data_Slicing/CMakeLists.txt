cmake_minimum_required(VERSION 3.10)

project(data_slicing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(data_slicing
    main.cpp
    FileSlicer.cpp
    TaskQueue.cpp
    ReaderWorker.cpp
    WriterWorker.cpp
    ChunkManager.cpp
)
