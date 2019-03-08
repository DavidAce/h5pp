
#include <iostream>
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



// Store some dummy data to an hdf5 file



int main()
{
    using cplx = std::complex<double>;

    static_assert(h5pp::Type::Check::hasMember_data<std::vector<double>>() and "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    std::string outputFilename      = "output/readWrite.h5";
    bool        createDir = true;
    size_t      logLevel  = 0;
    h5pp::File file(outputFilename, h5pp::AccessMode::TRUNCATE,createDir,logLevel);


    // Generate dummy data
    std::vector<double> vectorDouble = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                    -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};
    std::vector<cplx> vectorComplex = {-0.191154 + 0.326211i, 0.964728-0.712335i, -0.0351791-0.10264i,0.177544+0.99999i};

    Eigen::Tensor<cplx,4> tensorComplex(3,3,3,3);
    tensorComplex.setRandom();
    std::string stringDummy = "Dummy string with spaces";



    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;
    file.writeDataset(vectorDouble, "vectorDouble");

    std::cout << "Writing vectorComplex     : \n" <<  vectorComplex  << std::endl;
    file.writeDataset(vectorComplex, "vectorComplex");

    std::cout << "Writing tensorComplex     : \n" <<  tensorComplex  << std::endl;
    file.writeDataset(tensorComplex, "tensorComplex");

    std::cout << "Writing stringDummy       : \n" <<  stringDummy    << std::endl;
    file.writeDataset(stringDummy, "stringDummy");


    // Read the data back
    std::vector<double>     vectorDoubleRead;
    std::vector<cplx>       vectorComplexRead;
    Eigen::Tensor<cplx,4>   tensorComplexRead;
    std::string             stringDummyRead;


    std::cout << "Reading vectorDouble: " <<  vectorDoubleRead  << std::endl;
    file.readDataset(vectorDoubleRead, "vectorDouble");
    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}


    std::cout << "Reading vectorComplex: " <<  vectorComplexRead  << std::endl;
    file.readDataset(vectorComplexRead, "vectorComplex");
    if (vectorComplex != vectorComplexRead){throw std::runtime_error("vectorComplex != vectorComplexRead");}


    std::cout << "Reading stringDummy: " <<  stringDummyRead  << std::endl;
    file.readDataset(stringDummyRead, "stringDummy");
    if (stringDummy != stringDummyRead){throw std::runtime_error("stringDummy != stringDummyRead");}


    std::cout << "Reading tensorComplex: " <<  tensorComplexRead  << std::endl;
    file.readDataset(tensorComplexRead, "tensorComplex");
    //Tensor comparison isn't as straightforward if we want to properly test storage orders
    Eigen::Map<Eigen::VectorXcd> tensorMap(tensorComplex.data(), tensorComplex.size());
    Eigen::Map<Eigen::VectorXcd> tensorMapRead(tensorComplexRead.data(), tensorComplexRead.size());

    for (int i = 0; i < tensorComplex.dimension(0); i++ ) {
        for (int j = 0; j < tensorComplex.dimension(1); j++ ) {
            for (int k = 0; k < tensorComplex.dimension(2); k++ ) {
                for (int l = 0; l < tensorComplex.dimension(3); l++ ) {
                    std::cout << "[ " << i << "  " << j << " " << k << " " << l << " ]: " << tensorComplex(i,j,k,l) << "   " << tensorComplexRead(i,j,k,l) << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }

    if (tensorMap != tensorMapRead){throw std::runtime_error("tensorMap != tensorMapRead");}

    return 0;
}