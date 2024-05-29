#!/bin/bash

echo "Creating and navigating to the build directory..."
mkdir -p build
cd build || { echo "Failed to change directory to 'build'"; exit 1; }

echo "Running cmake..."
cmake .. || { echo "CMake failed"; exit 1; }

echo "Running make..."
make || { echo "Make failed"; exit 1; }

echo "Starting the server..."
./server &

echo "Opening a new terminal for the client..."
gnome-terminal -- bash -c "./client 127.0.0.1 8080; exec bash" || { echo "Failed to open new terminal"; exit 1; }

echo "Script completed."
wait
