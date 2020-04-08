
#include <h5pp/h5pp.h>
#include <iostream>

/*! \brief Prints the content of a vector nicely */
template<typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
    if(!v.empty()) {
        out << "[ ";
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
        out << "]";
    }
    return out;
}

// Store some dummy data to an hdf5 file

int main() {

    std::string outputFilename = "output/readWriteText.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, H5F_ACC_TRUNC | H5F_ACC_RDWR, logLevel);


    // Generate dummy data
    std::string stringDummy = "Dummy string";
    char charDummy[100] = "Dummy char array";

    //Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy");
    file.writeDataset(charDummy, "charDummy");
    file.writeDataset("Dummy string literal", "literalDummy");

    auto stringDummyRead = file.readDataset<std::string>("stringDummy");
    if(stringDummy != stringDummyRead)
        throw std::runtime_error(h5pp::format("String dummy failed: [{}] != [{}]", stringDummy, stringDummyRead));

    auto charDummyRead = file.readDataset<std::string>("charDummy");
    if(strcmp(charDummy, charDummyRead.c_str()) != 0 )
        throw std::runtime_error(h5pp::format("Char dummy failed: [{}] != [{}]", charDummy, charDummyRead));

    auto literalDummy = file.readDataset<std::string>("literalDummy");
    if("Dummy string literal" != literalDummy)
        throw std::runtime_error(h5pp::format("Literal dummy failed: [{}] != [{}]", "Dummy string literal", literalDummy));


    char charBuffer[100];
    file.readDataset(charBuffer,{99},"charDummy");


    return 0;
}