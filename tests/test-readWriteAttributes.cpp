
#include <complex>
#include <h5pp/h5pp.h>

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

int main() {
    // Generate dummy data
    int                               AttributeInt                 = 7;
    double                            AttributeDouble              = 47.4;
    std::complex<int>                 AttributeComplexInt          = {47, -10};
    std::complex<double>              AttributeComplexDouble       = {47.2, -10.2445};
    std::array<long, 4>               AttributeArrayLong           = {1, 2, 3, 4};
    float                             AttributeCArrayFloat[4]      = {1, 2, 3, 4};
    std::vector<std::complex<double>> AttributeVectorComplexDouble = {
        {  2.0,  5.0},
        {  3.1, -2.3},
        3.0,
        {-51.2,    5}
    };
    std::vector<double> AttributeVectorDouble = {1.0,  0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
                                                 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 0.0,
                                                 -1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0};
    std::string         AttributeString       = "This is a very long string that I am testing";
    char                AttributeCharArray[]  = "This is a char array";

    // define the file
    std::string outputFilename = "output/readWriteAttributes.h5";
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
    file.writeAttribute(AttributeVectorDouble, "testGroup/vectorDouble", "AttributeVectorDouble"); // Try overwrite
    file.writeAttribute(AttributeVectorComplexDouble, "testGroup/vectorDouble", "AttributeVectorComplexDouble");
    file.writeAttribute(AttributeString, "testGroup/vectorDouble", "AttributeString");
    file.writeAttribute(AttributeString, "testGroup/vectorDouble", "AttributeString"); // Try overwrite
    file.writeAttribute(AttributeCharArray, "testGroup/vectorDouble", "AttributeCharArray");
    // Read the data back
    auto ReadAttributeInt           = file.readAttribute<int>("testGroup/vectorDouble", "AttributeInt");
    auto ReadAttributeDouble        = file.readAttribute<double>("testGroup/vectorDouble", "AttributeDouble");
    auto ReadAttributeComplexInt    = file.readAttribute<std::complex<int>>("testGroup/vectorDouble", "AttributeComplexInt");
    auto ReadAttributeComplexDouble = file.readAttribute<std::complex<double>>("testGroup/vectorDouble", "AttributeComplexDouble");
    auto ReadAttributeArrayLong     = file.readAttribute<std::array<long, 4>>("testGroup/vectorDouble", "AttributeArrayLong");
    auto ReadAttributeCArrayFloat   = file.readAttribute<std::vector<float>>("testGroup/vectorDouble", "AttributeCArrayFloat");
    auto ReadAttributeVectorDouble  = file.readAttribute<std::vector<double>>("testGroup/vectorDouble", "AttributeVectorDouble");
    auto ReadAttributeVectorComplexDouble =
        file.readAttribute<std::vector<std::complex<double>>>("testGroup/vectorDouble", "AttributeVectorComplexDouble");
    auto ReadAttributeString    = file.readAttribute<std::string>("testGroup/vectorDouble", "AttributeString");
    auto ReadAttributeCharArray = file.readAttribute<std::string>("testGroup/vectorDouble", "AttributeCharArray");

    if(ReadAttributeInt != AttributeInt) throw std::runtime_error("ReadAttributeInt != AttributeInt");
    if(ReadAttributeDouble != AttributeDouble) throw std::runtime_error("ReadAttributeDouble != AttributeDouble");
    if(ReadAttributeComplexInt != AttributeComplexInt) throw std::runtime_error("ReadAttributeComplexInt != AttributeComplexInt");
    if(ReadAttributeComplexDouble != AttributeComplexDouble)
        throw std::runtime_error("ReadAttributeComplexDouble != AttributeComplexDouble");
    if(ReadAttributeArrayLong != AttributeArrayLong) throw std::runtime_error("ReadAttributeArrayLong != AttributeArrayLong");
    if(not std::equal(ReadAttributeCArrayFloat.begin(), ReadAttributeCArrayFloat.end(), std::begin(AttributeCArrayFloat)))
        throw std::runtime_error("ReadAttributeCArrayFloat                != AttributeCArrayFloat)            ");

    if(ReadAttributeVectorDouble != AttributeVectorDouble) throw std::runtime_error("ReadAttributeVectorDouble != AttributeVectorDouble");
    if(ReadAttributeVectorComplexDouble != AttributeVectorComplexDouble)
        throw std::runtime_error("ReadAttributeVectorComplexDouble != AttributeVectorComplexDouble");
    if(ReadAttributeString != AttributeString) throw std::runtime_error("ReadAttributeString != AttributeString");
    if(ReadAttributeCharArray != AttributeCharArray) throw std::runtime_error("ReadAttributeCharArray != AttributeCharArray");

#ifdef H5PP_USE_EIGEN3
    static_assert(h5pp::type::sfinae::has_Scalar_v<Eigen::MatrixXd> and
                  "Compile time type-checker failed. Could not properly detect class member Scalar. Scan that you are "
                  "using a supported compiler!");
    // Generate dummy data
    Eigen::MatrixXd  AttributeEigenMatrixDouble(10, 10);
    Eigen::MatrixXcd AttributeEigenMatrixComplexDouble(10, 10);
    AttributeEigenMatrixDouble.setRandom();
    AttributeEigenMatrixComplexDouble.setRandom();

    // Write attributes
    file.writeAttribute(AttributeEigenMatrixDouble, "testGroup/vectorDouble", "AttributeEigenMatrixDouble");
    file.writeAttribute(AttributeEigenMatrixComplexDouble, "testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble");

    // Read the data back
    auto ReadAttributeEigenMatrixDouble = file.readAttribute<Eigen::MatrixXd>("testGroup/vectorDouble", "AttributeEigenMatrixDouble");
    auto ReadAttributeEigenMatrixComplexDouble =
        file.readAttribute<Eigen::MatrixXcd>("testGroup/vectorDouble", "AttributeEigenMatrixComplexDouble");

    if(ReadAttributeEigenMatrixDouble != AttributeEigenMatrixDouble)
        throw std::runtime_error("ReadAttributeEigenMatrixDouble != AttributeEigenMatrixDouble");
    if(ReadAttributeEigenMatrixComplexDouble != AttributeEigenMatrixComplexDouble)
        throw std::runtime_error("ReadAttributeEigenMatrixComplexDouble != AttributeEigenMatrixComplexDouble");
#endif

    auto allAttributes = file.getAttributeNames("testGroup/vectorDouble");
    h5pp::print("{}\n", allAttributes);
    return 0;
}