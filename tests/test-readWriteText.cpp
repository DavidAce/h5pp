
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

    std::vector<std::string> vecString;
    vecString.emplace_back("this is a variable");
    vecString.emplace_back("length array");
    file.writeDataset(vecString, "vecString");
    auto vecStringReadString = file.readDataset<std::string>("vecString");
    if(vecStringReadString != "this is a variable\nlength array")
        throw std::runtime_error(h5pp::format("String mismatch: [{}] != [{}]", vecString,vecStringReadString));
    auto vecstringReadVector = file.readDataset<std::vector<std::string>>("vecString");
    if(vecstringReadVector.size() != vecString.size())
        throw std::runtime_error(h5pp::format("Vecstring read size mismatch: [{}] != [{}]", vecString.size(), vecstringReadVector.size()));
    for(size_t i = 0; i < vecString.size(); i++)
        if(vecString[i] != vecstringReadVector[i]) throw std::runtime_error(h5pp::format("Vecstring read element mismatch: [{}] != [{}]", vecString[i], vecstringReadVector[i]));

    // Generate dummy data
    std::string stringDummy    = "Dummy string";
    char        charDummy[100] = "Dummy char array";

    // Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy");
    file.writeDataset(charDummy, "charDummy");
    file.writeDataset(charDummy, {1}, "charDummy_asarray_dims");
    file.writeDataset(charDummy, 1, "charDummy_asarray_size");
    // The following shouldn't work
    try{
        file.writeDataset(charDummy, {2}, "charDummy_asarray_dims");
    }catch(std::exception & err){

    }


    file.writeDataset("Dummy string literal", "literalDummy");

    auto stringDummyRead = file.readDataset<std::string>("stringDummy");
    if(stringDummy != stringDummyRead) throw std::runtime_error(h5pp::format("String dummy failed: [{}] != [{}]", stringDummy, stringDummyRead));

    auto charDummyRead = file.readDataset<std::string>("charDummy");
    if(strcmp(charDummy, charDummyRead.c_str()) != 0) throw std::runtime_error(h5pp::format("Char dummy failed: [{}] != [{}]", charDummy, charDummyRead));

    auto literalDummy = file.readDataset<std::string>("literalDummy");
    if("Dummy string literal" != literalDummy) throw std::runtime_error(h5pp::format("Literal dummy failed: [{}] != [{}]", "Dummy string literal", literalDummy));



    // Now try some outlier cases

    // Write text data in various ways
    file.writeDataset(stringDummy, "stringDummy_chunked", H5D_CHUNKED);
    file.writeDataset("some other dummy text that makes it longer", "stringDummy_chunked");

    file.writeDataset(stringDummy, "stringDummy_extended");
    file.writeDataset("some other dummy text that makes it longer", "stringDummy_extended");
    auto stringDummy_extended = file.readDataset<std::string>("stringDummy_extended");
    if(stringDummy_extended != "some other dummy text that makes it longer")
        throw std::runtime_error(h5pp::format("Failed to extend string: [{}]",stringDummy_extended));

//    char stringDummyChar[stringDummyRead.size() + 1];
//    stringDummyRead.copy(stringDummyChar, stringDummyRead.size() + 1);
    return 0;
}