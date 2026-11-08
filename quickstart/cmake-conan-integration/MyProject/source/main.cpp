#include <iostream>
#include <string>
#include <h5pp/h5pp.h>

int main(){
    h5pp::File file("MyProject-output/quickstart.h5", h5pp::FileAccess::REPLACE);
    // Write "Hello world" using a v1 API call retained in h5pp 2.
    file.writeDataset("Hello World", "myFirstDataset");

    // Read the message back using the v1 API
    auto message = file.readDataset<std::string>("myFirstDataset");

    std::cout << "Successfully wrote: [" << message << "] to file: " << file.getFilePath() << std::endl;
}
