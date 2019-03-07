
#include <iostream>
#include <h5pp/h5pp.h>

int main(){

    h5pp::File file("someFile.h5", "output",h5pp::AccessMode::TRUNCATE,true,0);
    using namespace std::complex_literals;

    std::string                         String         = "This is a string";
    char                                Char[100]      = "This is a char array";
    double                              Double              (2);
    std::complex<double>                ComplexDouble       (3,4);
    int                                 arrayInt[5]         = {1,2,3,4,5};
    std::vector<int>                    vectorInt           (10, 42);
    std::vector<long>                   vectorLong          (10, 42);
    std::vector<unsigned int>           vectorUint          (10, 42);
    std::vector<unsigned long>          vectorUlong         (10, 42);
    std::vector<float >                 vectorFloat         (10, 42);
    std::vector<double>                 vectorDouble        (10, 42);
    std::vector<std::complex<int>>      vectorComplexInt    (10, std::complex<int>(42,7));
    std::vector<std::complex<double>>   vectorComplexDouble (10, 10.0 + 5.0i);
    Eigen::MatrixXi                     matrixInt           (2,2);
    Eigen::MatrixXd                     matrixDouble        (2,2);
    Eigen::MatrixXcd                    matrixComplexDouble (2,2);
    matrixInt           << 1,2,3,4;
    matrixDouble        << 1.5,2.5,3.5,4.5;
    matrixComplexDouble << 1.0 + 2.0i,  3.0 + 4.0i, 5.0+6.0i , 7.0+8.0i;
    file.write_dataset(String             ,"simpleWriteGroup/String");
    file.write_dataset(Char               ,"simpleWriteGroup/Char");
    file.write_dataset(Double             ,"simpleWriteGroup/Double");
    file.write_dataset(ComplexDouble      ,"simpleWriteGroup/ComplexDouble");
    file.write_dataset(arrayInt           ,"simpleWriteGroup/arrayInt");
    file.write_dataset(vectorInt          ,"simpleWriteGroup/vectorInt");
    file.write_dataset(vectorLong         ,"simpleWriteGroup/vectorLong");
    file.write_dataset(vectorUint         ,"simpleWriteGroup/vectorUint");
    file.write_dataset(vectorUlong        ,"simpleWriteGroup/vectorUlong");
    file.write_dataset(vectorFloat        ,"simpleWriteGroup/vectorFloat");
    file.write_dataset(vectorDouble       ,"simpleWriteGroup/vectorDouble");
    file.write_dataset(vectorComplexInt   ,"simpleWriteGroup/vectorComplexInt");
    file.write_dataset(vectorComplexDouble,"simpleWriteGroup/vectorComplexDouble");
    file.write_dataset(matrixInt          ,"simpleWriteGroup/matrixInt");
    file.write_dataset(matrixDouble       ,"simpleWriteGroup/matrixDouble");
    file.write_dataset(matrixComplexDouble,"simpleWriteGroup/matrixComplexDouble");

    Eigen::Matrix<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<double>,Eigen::Dynamic,Eigen::Dynamic> test =  matrixComplexDouble.transpose().cast<h5pp::Type::Complex::H5T_COMPLEX_STRUCT<double>>();
    file.write_dataset(test,"simpleWriteGroup/test");


    return 0;
}
