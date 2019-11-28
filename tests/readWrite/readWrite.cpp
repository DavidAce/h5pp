
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
    size_t      logLevel  = 0;
    h5pp::File file(outputFilename,h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE,logLevel);


    // Generate dummy data
    std::string stringDummy = "Dummy string with spaces";


    std::vector<double> vectorDouble = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                    -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};
    std::vector<cplx> vectorComplex = {-0.191154 + 0.326211i, 0.964728-0.712335i, -0.0351791-0.10264i,0.177544+0.99999i};

    Eigen::MatrixXd matrixDouble(3,2);
    matrixDouble.setRandom();

    Eigen::Matrix<int,2,2,Eigen::RowMajor> matrixIntRowMajor;
    matrixIntRowMajor.setRandom();

    Eigen::Tensor<cplx,4> tensorComplex(2,3,2,3);
    tensorComplex.setRandom();
    Eigen::Tensor<double,3> tensorDoubleRowMajor(2,3,4);
    tensorDoubleRowMajor.setRandom();


    std::cout << "Writing stringDummy       : \n" <<  stringDummy    << std::endl;
    file.writeDataset(stringDummy, "stringDummy");

    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;
    file.writeDataset(vectorDouble, "vectorDouble");

    std::cout << "Writing vectorComplex     : \n" <<  vectorComplex  << std::endl;
    file.writeDataset(vectorComplex, "vectorComplex");

    std::cout << "Writing matrixDouble     : \n" <<  matrixDouble  << std::endl;
    file.writeDataset(matrixDouble,{matrixDouble.rows(), matrixDouble.cols()}, "matrixDouble");


    std::cout << "Writing matrixIntRowMajor     : \n" <<  matrixIntRowMajor  << std::endl;
    file.writeDataset(matrixIntRowMajor,{matrixIntRowMajor.rows(), matrixIntRowMajor.cols()} , "matrixIntRowMajor");


    std::cout << "Writing tensorComplex     : \n" <<  tensorComplex  << std::endl;
    file.writeDataset(tensorComplex, "tensorComplex");

    std::cout << "Writing tensorDoubleRowMajor     : \n" <<  tensorDoubleRowMajor  << std::endl;
    file.writeDataset(tensorDoubleRowMajor, "tensorDoubleRowMajor");


    auto foundLinksInRoot = file.getContentsOfGroup("/");
    for (auto & link : foundLinksInRoot){
        std::cout << "Found Link: " << link << std::endl;
    }

    // Read the data back
    std::string               stringDummyRead;

    std::vector<double>     vectorDoubleRead;
    std::vector<cplx>       vectorComplexRead;

    Eigen::MatrixXd matrixDoubleRead;
    Eigen::Matrix<int,2,2,Eigen::RowMajor> matrixIntRowMajorRead;

    Eigen::Tensor<cplx,4>     tensorComplexRead;
    Eigen::Tensor<double,3>   tensorDoubleRowMajorRead;


    std::cout << "Reading stringDummy: \n";
    file.readDataset(stringDummyRead, "stringDummy");
    std::cout << stringDummyRead << std::endl;
    if (stringDummy != stringDummyRead){throw std::runtime_error("stringDummy != stringDummyRead");}

    std::cout << "Reading vectorDouble: \n";
    file.readDataset(vectorDoubleRead, "vectorDouble");
    std::cout << vectorDoubleRead << std::endl;
    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}

    std::cout << "Reading vectorComplex: \n";
    file.readDataset(vectorComplexRead, "vectorComplex");
    std::cout << vectorComplexRead << std::endl;
    if (vectorComplex != vectorComplexRead){throw std::runtime_error("vectorComplex != vectorComplexRead");}

    std::cout << "Reading matrixDouble: \n";
    file.readDataset(matrixDoubleRead, "matrixDouble");
    std::cout << matrixDoubleRead << std::endl;
    if (matrixDouble != matrixDoubleRead){throw std::runtime_error("matrixDouble != matrixDoubleRead");}


    std::cout << "Reading matrixIntRowMajor: \n";
    file.readDataset(matrixIntRowMajorRead, "matrixIntRowMajor");
    std::cout << matrixIntRowMajorRead << std::endl;
    if (matrixIntRowMajor != matrixIntRowMajorRead){throw std::runtime_error("matrixIntRowMajor != matrixIntRowMajorRead");}




    std::cout << "Reading tensorComplex: \n";
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

    if (tensorMap != tensorMapRead){throw std::runtime_error("tensorComplex != tensorComplexRead");}


    std::cout << "Reading tensorDoubleRowMajor: \n";
    file.readDataset(tensorDoubleRowMajorRead, "tensorDoubleRowMajor");
    //Tensor comparison isn't as straightforward if we want to properly test storage orders
    Eigen::Map<Eigen::VectorXd> tensorMap2(tensorDoubleRowMajor.data(), tensorDoubleRowMajor.size());
    Eigen::Map<Eigen::VectorXd> tensorMapRead2(tensorDoubleRowMajorRead.data(), tensorDoubleRowMajorRead.size());

    for (int i = 0; i < tensorDoubleRowMajor.dimension(0); i++ ) {
        for (int j = 0; j < tensorDoubleRowMajor.dimension(1); j++ ) {
            for (int k = 0; k < tensorDoubleRowMajor.dimension(2); k++ ) {
                std::cout << "[ " << i << "  " << j << " " << k << " ]: " << tensorDoubleRowMajor(i,j,k) << "   " << tensorDoubleRowMajorRead(i,j,k) << std::endl;
            }
            std::cout << std::endl;
        }
    }

    if (tensorMap2 != tensorMapRead2){throw std::runtime_error("tensorDoubleRowMajor != tensorDoubleRowMajorRead");}


    auto dims        = file.getDatasetDims("vectorDouble");
    auto * preAllocatedDouble = new double [dims[0]];
    file.readDataset(preAllocatedDouble,dims[0], "vectorDouble");
    delete[] preAllocatedDouble;
    return 0;
}