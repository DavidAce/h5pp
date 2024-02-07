
#include <h5pp/h5pp.h>

int main() {
    static_assert(
        h5pp::type::sfinae::has_data_v<std::vector<double>> and
        "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    // Generate dummy data
    int                               AttributeInt                 = 7;
    double                            AttributeDouble              = 47.4;
    std::complex<int>                 AttributeComplexInt          = {47, -10};
    std::complex<double>              AttributeComplexDouble       = {47.2, -10.2445};
    std::array<long, 4>               AttributeArrayLong           = {1, 2, 3, 4};
    float                             AttributeCArrayFloat[4]      = {1, 2, 3, 4};
    std::vector<std::complex<double>> AttributeVectorComplexDouble = {{2.0, 5.0}, {3.1, -2.3}, 3.0, {-51.2, 5}};
    std::vector<double>               AttributeVectorDouble        = {1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                                      0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,
                                                                      -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::string                       AttributeString              = "This is a very long string that I am testing";
    char                              AttributeCharArray[]         = "This is a char array";

    // define the file
    std::string outputFilename = "output/typeInfo.h5";
    size_t      logLevel       = 0;
    h5pp::File  file(outputFilename, h5pp::FileAccess::REPLACE, logLevel);

    // Write dataset
    file.writeDataset(std::vector<double>(10, 5), "testGroup/vectorDouble");
    // Write attributes

    file.writeAttribute(AttributeInt, "testGroup/vectorDouble", "AttributeInt");
    file.writeAttribute(AttributeDouble, "testGroup/vectorDouble", "AttributeDouble");
    file.writeAttribute(AttributeComplexInt, "testGroup/vectorDouble", "AttributeComplexInt");
    file.writeAttribute(AttributeComplexDouble, "testGroup/vectorDouble", "AttributeComplexDouble");
    file.writeAttribute(AttributeArrayLong, "testGroup/vectorDouble", "AttributeArrayLong");
    file.writeAttribute(AttributeCArrayFloat, "testGroup/vectorDouble", "AttributeCArrayFloat");
    file.writeAttribute(AttributeVectorDouble, "testGroup/vectorDouble", "AttributeVectorDouble");
    file.writeAttribute(AttributeVectorComplexDouble, "testGroup/vectorDouble", "AttributeVectorComplexDouble");
    file.writeAttribute(AttributeString, "testGroup/vectorDouble", "AttributeString");
    file.writeAttribute(AttributeCharArray, "testGroup/vectorDouble", "AttributeCharArray");

    for(auto &info : file.getTypeInfoAttributes("testGroup/vectorDouble")) h5pp::print("{}\n", info.string());

    auto info = file.getTypeInfoDataset("testGroup/vectorDouble");
    h5pp::print("{}\n", info.string());

    return 0;
}