
#include <iostream>
#include <complex>
#include <h5pp/h5pp.h>
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

using namespace std::complex_literals;


int main()
{
    using cplx = std::complex<double>;

    static_assert(h5pp::Type::Check::hasMember_data<std::vector<double>>() and "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    // Generate dummy data
    std::vector<double> vectorDouble = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                    -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};



    std::string outputFilename      = "output/readWriteAttributes.h5";
    size_t      logLevel  = 0;
    h5pp::File file(outputFilename,h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE,logLevel);


    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;

    file.writeDataset(vectorDouble, "testGroup/vectorDouble");

    file.writeAttributeToLink(std::string("This is a string"), "AttributeString", "testGroup/vectorDouble");
    file.writeAttributeToLink("This is a char array", "AttributeCharArray", "testGroup/vectorDouble");
    file.writeAttributeToLink(1, "AttributeInt", "testGroup/vectorDouble");
    file.writeAttributeToLink(1.0, "AttributeDouble", "testGroup/vectorDouble");
    file.writeAttributeToLink(std::complex<double>(2, 3), "AttributeComplexDouble", "testGroup/vectorDouble");
    file.writeAttributeToLink((int[]) {1, 2, 3, 4, 5, 6}, "AttributeIntArray", "testGroup/vectorDouble");
    file.writeAttributeToLink(vectorDouble, "AttributeVectorDouble", "testGroup/vectorDouble");


    // Read the data back
    std::vector<double>     vectorDoubleRead;
    std::cout << "Reading testGroup/vectorDouble: " <<  vectorDoubleRead  << std::endl;
    file.readDataset(vectorDoubleRead, "testGroup/vectorDouble");
    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}

    return 0;
}