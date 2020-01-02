
#include <iostream>
#include <complex>
#include <h5pp/h5pp.h>
#include <gitversion.h>
/*! \brief Prints the content of a vector nicely */
template<typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
    if (!v.empty()) {
        out << "[ ";
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
        out << "]";
    }
    return out;
}



int main()
{

    static_assert(h5pp::Type::Check::hasMember_data<std::vector<double>>() and "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    // Generate dummy data
    int                                 AttributeInt = 7;
    double                              AttributeDouble = 47.4;
    std::complex<int>                   AttributeComplexInt     = {47, -10};
    std::complex<double>                AttributeComplexDouble  = {47.2, -10.2445};
    std::array<long,4>                  AttributeArrayLong      = {1,2,3,4};
    float                               AttributeCArrayFloat[4] = {1,2,3,4};
    std::vector<double>                 AttributeVectorDouble =
                                                       { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                                         0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                                        -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};

    std::vector<std::complex<double>>   AttributeVectorComplexDouble = { {2.0,5.0} , {3.1,-2.3}, 3.0,{-51.2, 5} };
    Eigen::MatrixXd                     AttributeEigenMatrixDouble(10,10);             AttributeEigenMatrixDouble.setRandom();
    Eigen::MatrixXcd                    AttributeEigenMatrixComplexDouble(10,10);      AttributeEigenMatrixComplexDouble.setRandom();
    std::string                         AttributeString     = "This is a very long string that I am testing";
    char                                AttributeCharArray[]= "This is a char array";
    // define the file
    std::string outputFilename      = "output/readWriteAttributes.h5";
    size_t      logLevel  = 0;
    h5pp::File file(outputFilename,h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE,logLevel);

    // Write dataset
    file.writeDataset(std::vector<double>(10,5), "testGroup/vectorDouble");
    // Write attributes


    file.writeAttribute(AttributeInt, "AttributeInt", "testGroup/vectorDouble");
    file.writeAttribute(AttributeDouble, "AttributeDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeComplexInt, "AttributeComplexInt", "testGroup/vectorDouble");
    file.writeAttribute(AttributeComplexDouble, "AttributeComplexDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeArrayLong, "AttributeArrayLong", "testGroup/vectorDouble");
    file.writeAttribute(AttributeCArrayFloat, "AttributeCArrayFloat", "testGroup/vectorDouble");
    file.writeAttribute(AttributeVectorDouble, "AttributeVectorDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeVectorComplexDouble, "AttributeVectorComplexDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeEigenMatrixDouble, "AttributeEigenMatrixDouble", "testGroup/vectorDouble");
    file.writeAttribute(AttributeEigenMatrixComplexDouble, "AttributeEigenMatrixComplexDouble",
                        "testGroup/vectorDouble");
    file.writeAttribute(AttributeString, "AttributeString", "testGroup/vectorDouble");
    file.writeAttribute(AttributeCharArray, "AttributeCharArray", "testGroup/vectorDouble");
//    return 0;

    // Read the data back
    auto ReadAttributeInt                       = file.readAttribute<int>                               ("AttributeInt", "testGroup/vectorDouble");
    auto ReadAttributeDouble                    = file.readAttribute<double>                            ("AttributeDouble", "testGroup/vectorDouble");
    auto ReadAttributeComplexInt                = file.readAttribute<std::complex<int>>                 ("AttributeComplexInt", "testGroup/vectorDouble");
    auto ReadAttributeComplexDouble             = file.readAttribute<std::complex<double>>              ("AttributeComplexDouble", "testGroup/vectorDouble");
    auto ReadAttributeArrayLong                 = file.readAttribute<std::array<long,4>>                ("AttributeArrayLong", "testGroup/vectorDouble");
    auto ReadAttributeCArrayFloat               = file.readAttribute<std::vector<float>>                ("AttributeCArrayFloat", "testGroup/vectorDouble");
    auto ReadAttributeVectorDouble              = file.readAttribute<std::vector<double>>               ("AttributeVectorDouble", "testGroup/vectorDouble");
    auto ReadAttributeVectorComplexDouble       = file.readAttribute<std::vector<std::complex<double>>> ("AttributeVectorComplexDouble", "testGroup/vectorDouble");
    auto ReadAttributeEigenMatrixDouble         = file.readAttribute<Eigen::MatrixXd>                   ("AttributeEigenMatrixDouble", "testGroup/vectorDouble");
    auto ReadAttributeEigenMatrixComplexDouble  = file.readAttribute<Eigen::MatrixXcd>                  ("AttributeEigenMatrixComplexDouble", "testGroup/vectorDouble");
    auto ReadAttributeString                    = file.readAttribute<std::string>                       ("AttributeString", "testGroup/vectorDouble");
    auto ReadAttributeCharArray                 = file.readAttribute<std::string>                       ("AttributeCharArray", "testGroup/vectorDouble");

    if(ReadAttributeInt                        != AttributeInt)                     throw std::runtime_error("ReadAttributeInt                        != AttributeInt)                    ");
    if(ReadAttributeDouble                     != AttributeDouble)                  throw std::runtime_error("ReadAttributeDouble                     != AttributeDouble)                 ");
    if(ReadAttributeComplexInt                 != AttributeComplexInt)              throw std::runtime_error("ReadAttributeComplexInt                 != AttributeComplexInt)             ");
    if(ReadAttributeComplexDouble              != AttributeComplexDouble)           throw std::runtime_error("ReadAttributeComplexDouble              != AttributeComplexDouble)          ");
    if(ReadAttributeArrayLong                  != AttributeArrayLong)               throw std::runtime_error("ReadAttributeArrayLong                  != AttributeArrayLong)              ");
    if(not std::equal(ReadAttributeCArrayFloat.begin(),ReadAttributeCArrayFloat.end(), std::begin(AttributeCArrayFloat))){
        throw std::runtime_error("ReadAttributeCArrayFloat                != AttributeCArrayFloat)            ");
    }

    if(ReadAttributeVectorDouble               != AttributeVectorDouble)            throw std::runtime_error("ReadAttributeVectorDouble               != AttributeVectorDouble)           ");
    if(ReadAttributeVectorComplexDouble        != AttributeVectorComplexDouble)     throw std::runtime_error("ReadAttributeVectorComplexDouble        != AttributeVectorComplexDouble)    ");
    if(ReadAttributeEigenMatrixDouble          != AttributeEigenMatrixDouble)       throw std::runtime_error("ReadAttributeEigenMatrixDouble          != AttributeEigenMatrixDouble)      ");
    if(ReadAttributeEigenMatrixComplexDouble   != AttributeEigenMatrixComplexDouble)throw std::runtime_error("ReadAttributeEigenMatrixComplexDouble   != AttributeEigenMatrixComplexDouble");
    if(ReadAttributeString                     != AttributeString)                  throw std::runtime_error("ReadAttributeString                     != AttributeString)                 ");
    if(ReadAttributeCharArray                  != AttributeCharArray)               throw std::runtime_error("ReadAttributeCharArray                  != AttributeCharArray)              ");


    auto allAttributes = file.getAttributeNames("testGroup/vectorDouble");
    for(auto & attr : allAttributes)std::cout << attr << std::endl;

    return 0;
}