#include <iostream>
#include <fstream>

int main() {
    std::ofstream file("debug_output.txt");
    file << "Test suite starting..." << std::endl;
    file << "Console output test" << std::endl;
    file.close();
    
    std::cout << "Hello from Bloomberg Test Suite!" << std::endl;
    std::cout << "This should appear on console" << std::endl;
    
    return 0;
}
