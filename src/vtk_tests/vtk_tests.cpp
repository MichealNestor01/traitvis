#include <vtkVersion.h>
#include <iostream>
#include <string>

int main() {
    std::string vtkVersion = vtkVersion::GetVTKVersion();

    std::cout << "VTK Version: " << vtkVersion << std::endl;
    return 0;
}