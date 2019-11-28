
#include <iostream>
#include <complex>
#include <h5pp/h5pp.h>

int main(){

    std::string outputFilename      = "output/overWrite.h5";
    size_t      logLevel  = 1;
    h5pp::File file(outputFilename,h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE,logLevel);


    using namespace std::complex_literals;
    std::vector<std::complex<double>>   vectorComplexDouble (10000, 10.0 + 5.0i);
    Eigen::MatrixXi                     matrixInt           = Eigen::MatrixXi::Random (100,100);
    Eigen::MatrixXd                     matrixDouble        = Eigen::MatrixXd::Random (100,100);
    Eigen::MatrixXcd                    matrixComplexDouble = Eigen::MatrixXcd::Random(100,100);
    std::string                         somestring = "this is a teststring";


    file.enableDefaultExtendable();

    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(matrixInt,           "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble,        "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");
    file.writeDataset(somestring,          "overWriteGroup_ext_enabled/somestring");

    // Now overwrite

    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(matrixInt,           "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble,        "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");
    file.writeDataset(somestring,          "overWriteGroup_ext_enabled/somestring");


    //Now increase size and overwrite again
    vectorComplexDouble = std::vector<std::complex<double>> (15000, 10.0 + 5.0i);
    matrixInt           = Eigen::MatrixXi::Random (200,200);
    matrixDouble        = Eigen::MatrixXd::Random (200,200);
    matrixComplexDouble = Eigen::MatrixXcd::Random(200,200);
    somestring          = "this is a slightly longer string";

    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(matrixInt,           "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble,        "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");
    file.writeDataset(somestring,          "overWriteGroup_ext_enabled/somestring");


    //Now decrease size and overwrite again
    vectorComplexDouble = std::vector<std::complex<double>> (15000, 10.0 + 5.0i);
    matrixInt           = Eigen::MatrixXi::Random (20,20);
    matrixDouble        = Eigen::MatrixXd::Random (20,20);
    matrixComplexDouble = Eigen::MatrixXcd::Random(20,20);
    somestring          = "short string";

    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_enabled/vectorComplexDouble");
    file.writeDataset(matrixInt,           "overWriteGroup_ext_enabled/matrixInt");
    file.writeDataset(matrixDouble,        "overWriteGroup_ext_enabled/matrixDouble");
    file.writeDataset(matrixComplexDouble, "overWriteGroup_ext_enabled/matrixComplexDouble");
    file.writeDataset(somestring,          "overWriteGroup_ext_enabled/somestring");


    // All the previous datasets were extendable, and if we reached this points they have been overritten successfully.
    // Now we'll try making some non-extendable datasets and check that overwriting actually fails
    file.disableDefaultExtendable();

    vectorComplexDouble = std::vector<std::complex<double>> (1000, 10.0 + 5.0i);
    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled/vectorComplexDouble");

    try{
        vectorComplexDouble = std::vector<std::complex<double>> (10000, 10.0 + 5.0i);
        file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled/vectorComplexDouble");
    }catch (std::exception &ex){
        std::cout << "\n \t THE ERROR ABOVE IS PART OF THE TEST AND WAS EXPECTED: " << ex.what() << std::endl;
    }

    // Strings are a special case that shouldn't fail for non-e
    somestring          = "this is a teststring";
    file.writeDataset(somestring,          "overWriteGroup_ext_disabled/somestring");
    file.writeDataset(somestring,          "overWriteGroup_ext_disabled/somestring");



    // This time we should check that making large enough dataset defaults to extendable even if
    // we disabled default extendable
    vectorComplexDouble = std::vector<std::complex<double>> (32*1024, 10.0 + 5.0i);
    file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled_chunked/vectorComplexDouble");
    try{
        vectorComplexDouble = std::vector<std::complex<double>> (128*1024, 10.0 + 5.0i);
        file.writeDataset(vectorComplexDouble, "overWriteGroup_ext_disabled_chunked/vectorComplexDouble");

    }
    catch (std::exception &ex){
        throw std::runtime_error("Automatic change from contiguous to chunked when size is large failed: " + std::string(ex.what()));
    }
    return 0;
}
