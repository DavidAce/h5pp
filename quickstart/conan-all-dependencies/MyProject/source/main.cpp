#include <iostream>
#include <string>
#include <h5pp/h5pp.h>

int main(){
    h5pp::fs::path
    h5pp::File file("../../output/quickstart.h5", h5pp::FilePermission::REPLACE);
    // Write "Hello world" to the dataset "myFirstDataset"
    file.writeDataset("Hello World", "myFirstDataset");

    // Read the message back from the dataset
    auto message = file.readDataset<std::string>("myFirstDataset");

    std::cout << "Successfully wrote: [" << message << "] to the file ../../output/quickstart.h5" << std::endl;
}