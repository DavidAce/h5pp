
#include <h5pp/h5pp.h>
#include <iostream>

int main() {
    static_assert(h5pp::type::sfinae::has_data<std::vector<double>>() and
                  "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    // Generate dummy data
    int                               AttributeInt                 = 7;
    double                            AttributeDouble              = 47.4;
    std::complex<int>                 AttributeComplexInt          = {47, -10};
    std::complex<double>              AttributeComplexDouble       = {47.2, -10.2445};
    std::array<long, 4>               AttributeArrayLong           = {1, 2, 3, 4};
    float                             AttributeCArrayFloat[4]      = {1, 2, 3, 4};
    std::vector<std::complex<double>> AttributeVectorComplexDouble = {{2.0, 5.0}, {3.1, -2.3}, 3.0, {-51.2, 5}};
    std::vector<double>               AttributeVectorDouble        = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::string                       AttributeString              = "This is a very long string that I am testing";
    char                              AttributeCharArray[]         = "This is a char array";

    // define the file
    std::string outputFilename = "output/typeInfo.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE, logLevel);

    // Write dataset
    file.writeDataset(std::vector<double>(10, 5), "testGroup/vectorDouble");
    // Write attributes

    file.writeAttribute(AttributeInt, "AttributeInt", "testGroup/vectorDouble");
    file.writeAttribute(AttributeDouble, "AttributeDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeComplexInt, "AttributeComplexInt", "testGroup/vectorDouble");
    file.writeAttribute(AttributeComplexDouble, "AttributeComplexDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeArrayLong, "AttributeArrayLong", "testGroup/vectorDouble");
    file.writeAttribute(AttributeCArrayFloat, "AttributeCArrayFloat", "testGroup/vectorDouble");
    file.writeAttribute(AttributeVectorDouble, "AttributeVectorDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeVectorComplexDouble, "AttributeVectorComplexDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeString, "AttributeString", "testGroup/vectorDouble");
    file.writeAttribute(AttributeCharArray, "AttributeCharArray", "testGroup/vectorDouble");

    for(auto &info : file.getAttributeTypeInfoAll("testGroup/vectorDouble")) {
        std::cout << info.name() << ": type [" << info.type().name() << "] size: " << info.size() << std::endl;
    }

    auto info = file.getDatasetTypeInfo("testGroup/vectorDouble");
    std::cout << info.name() << ": type [" << info.type().name() << "] size: " << info.size() << std::endl;

    return 0;
}