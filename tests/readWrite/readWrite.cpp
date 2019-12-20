
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


// Store some dummy data to an hdf5 file

int main()
{
    using cplx = std::complex<double>;

    static_assert(h5pp::Type::Check::hasMember_data<std::vector<double>>() and "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    std::string outputFilename      = "output/readWrite.h5";
    size_t      logLevel  = 0;
    h5pp::File file(outputFilename,h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE,logLevel);


    std::vector<int> emptyVector;
    file.writeDataset(emptyVector,"emptyVector");
    file.readDataset(emptyVector,"emptyVector");


    // Generate dummy data
    std::string stringDummy = "Dummy string with spaces";
    std::cout << "Writing stringDummy       : \n" <<  stringDummy    << std::endl;
    // Write data data
    file.writeDataset(stringDummy, "stringDummy");
    std::cout << "Reading stringDummy: \n";
    // Read the data back
    std::string               stringDummyRead;
    file.readDataset(stringDummyRead, "stringDummy");
    std::cout << stringDummyRead << std::endl;
    // Compare result
    if (stringDummy != stringDummyRead){throw std::runtime_error("stringDummy != stringDummyRead");}



    // std::vector<double>
    std::vector<double> vectorDouble = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                    -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};
    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;
    file.writeDataset(vectorDouble, "vectorDouble");
    std::cout << "Reading vectorDouble: \n";
    auto vectorDoubleRead = file.readDataset<std::vector<double>>("vectorDouble");
    std::cout << vectorDoubleRead << std::endl;
    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}


    // std::vector<std::complex<double>>
    std::vector<cplx> vectorComplex = {{-0.191154, 0.326211}, {0.964728,-0.712335}, {-0.0351791,-0.10264},{0.177544,0.99999}};
    std::cout << "Writing vectorComplex     : \n" <<  vectorComplex  << std::endl;
    file.writeDataset(vectorComplex, "vectorComplex");
    std::vector<cplx>       vectorComplexRead;
    std::cout << "Reading vectorComplex: \n";
    file.readDataset(vectorComplexRead, "vectorComplex");
    std::cout << vectorComplexRead << std::endl;
    if (vectorComplex != vectorComplexRead){throw std::runtime_error("vectorComplex != vectorComplexRead");}




    // Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic, Eigen::ColMajor>
    Eigen::MatrixXd matrixDouble(3,2);
    matrixDouble.setRandom();
    std::cout << "Writing matrixDouble     : \n" <<  matrixDouble  << std::endl;
    file.writeDataset(matrixDouble,{matrixDouble.rows(), matrixDouble.cols()}, "matrixDouble");
    Eigen::MatrixXd matrixDoubleRead;
    std::cout << "Reading matrixDouble: \n";
    file.readDataset(matrixDoubleRead, "matrixDouble");
    std::cout << matrixDoubleRead << std::endl;
    if (matrixDouble != matrixDoubleRead){throw std::runtime_error("matrixDouble != matrixDoubleRead");}



    // Eigen::Matrix<size_t,Eigen::Dynamic,Eigen::Dynamic, Eigen::Rowmajor>
    Eigen::Matrix<size_t,2,2,Eigen::RowMajor> matrixSizeTRowMajor;
    matrixSizeTRowMajor.setRandom();
    std::cout << "Writing matrixSizeTRowMajor: \n" <<  matrixSizeTRowMajor  << std::endl;
    file.writeDataset(matrixSizeTRowMajor,{matrixSizeTRowMajor.rows(), matrixSizeTRowMajor.cols()} , "matrixSizeTRowMajor");
    std::cout << "Reading matrixSizeTRowMajor: \n";
    Eigen::Matrix<size_t,2,2,Eigen::RowMajor> matrixSizeTRowMajorRead;
    file.readDataset(matrixSizeTRowMajorRead, "matrixSizeTRowMajor");
    std::cout << matrixSizeTRowMajorRead << std::endl;
    if (matrixSizeTRowMajor != matrixSizeTRowMajorRead){throw std::runtime_error("matrixSizeTRowMajor != matrixSizeTRowMajorRead");}




    Eigen::Tensor<cplx,4> tensorComplex(2,3,2,3);
    tensorComplex.setRandom();
    std::cout << "Writing tensorComplex     : \n" <<  tensorComplex  << std::endl;
    file.writeDataset(tensorComplex, "tensorComplex");
    std::cout << "Reading tensorComplex: \n";
    Eigen::Tensor<cplx,4>     tensorComplexRead;
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



    Eigen::Tensor<double,3> tensorDoubleRowMajor(2,3,4);
    tensorDoubleRowMajor.setRandom();



    std::cout << "Writing tensorDoubleRowMajor: \n" <<  tensorDoubleRowMajor  << std::endl;
    file.writeDataset(tensorDoubleRowMajor, "tensorDoubleRowMajor");
    Eigen::Tensor<double,3>   tensorDoubleRowMajorRead;
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



    auto foundLinksInRoot = file.getContentsOfGroup("/");
    for (auto & link : foundLinksInRoot){
        std::cout << "Found Link: " << link << std::endl;
    }


    auto * cStyleDoubleArray = new double [10];
    for(int i = 0; i < 10; i++) cStyleDoubleArray[i] = (double)i;
    file.writeDataset(cStyleDoubleArray,10,"cStyleDoubleArray");
    auto * cStyleDoubleArrayRead = new double [10];
    file.readDataset(cStyleDoubleArrayRead,10, "cStyleDoubleArray");
    delete[] cStyleDoubleArray;
    delete[] cStyleDoubleArrayRead;




    // Test new field2 type
    struct Field2{
        double x ;
        double y ;
    };
    std::vector<Field2> field2array (10);
    for(size_t i = 0; i < field2array.size();i++){
        field2array[i].x = 2.3*i;
        field2array[i].y = 20.5*i;
    }
    file.writeDataset(field2array, "field2array");
    auto field2ReadArray = file.readDataset<std::vector<Field2>>("field2array");
    for(size_t i= 0; i < field2array.size();i++) {
        if(field2array[i].x != field2ReadArray[i].x) throw std::runtime_error("field2array != field2ReadArray at elem: " + std::to_string(i));
        if(field2array[i].y != field2ReadArray[i].y) throw std::runtime_error("field2array != field2ReadArray at elem: " + std::to_string(i));
    }


    Eigen::Matrix<Field2,Eigen::Dynamic,Eigen::Dynamic> field2Matrix (10,10);
    for(int row = 0; row < field2Matrix.rows(); row++){
        for(int col = 0; col < field2Matrix.cols(); col++){
            field2Matrix(row,col) = {(double)row,(double)col};
        }
    }
    file.writeDataset(field2Matrix, "field2Matrix");
    auto field2MatrixRead = file.readDataset<Eigen::Matrix<Field2,Eigen::Dynamic,Eigen::Dynamic> >("field2Matrix");
    for(long i= 0; i < field2Matrix.cols();i++) {
        for(long j= 0; j < field2Matrix.rows();j++) {
            if(field2Matrix(i,j).x != field2MatrixRead(i,j).x) throw std::runtime_error("field2array != field2ReadArray at elem: " + std::to_string(i) +" "+ std::to_string(j));
            if(field2Matrix(i,j).y != field2MatrixRead(i,j).y) throw std::runtime_error("field2array != field2ReadArray at elem: " + std::to_string(i) +" "+ std::to_string(j));
        }
    }

//    if(field2Matrix != field2MatrixRead) throw std::runtime_error("field2Matrix != field2MatrixRead");


    // Test new field3 type
    struct Field3{
        double x ;
        double y ;
        double z ;
    };
    std::vector<Field3> field3array (10);
    for(size_t i = 0; i < field3array.size();i++){
        field3array[i].x = 2.3*i;
        field3array[i].y = 20.5*i;
        field3array[i].z = 200.9*i;
    }
    file.writeDataset(field3array, "field3array");
    auto field3ReadArray = file.readDataset<std::vector<Field3>>("field3array");
    for(size_t i= 0; i < field3array.size();i++) {
        if(field3array[i].x != field3ReadArray[i].x) throw std::runtime_error("field3array.x != field3ReadArray.x at elem: " + std::to_string(i));
        if(field3array[i].y != field3ReadArray[i].y) throw std::runtime_error("field3array.y != field3ReadArray.y at elem: " + std::to_string(i));
        if(field3array[i].z != field3ReadArray[i].z) throw std::runtime_error("field3array.z != field3ReadArray.z at elem: " + std::to_string(i));
    }


    Eigen::Matrix<Field3,Eigen::Dynamic,Eigen::Dynamic> field3Matrix (10,10);
    for(int row = 0; row < field3Matrix.rows(); row++){
        for(int col = 0; col < field3Matrix.cols(); col++){
            field3Matrix(row,col) = {(double)row,(double)col, (double)(col+row)};
        }
    }
    file.writeDataset(field3Matrix, "field3Matrix");
    auto field3MatrixRead = file.readDataset<Eigen::Matrix<Field3,Eigen::Dynamic,Eigen::Dynamic>>("field3Matrix");
    for(long i = 0; i < field3Matrix.cols(); i++) {
        for(long j= 0; j < field3Matrix.rows(); j++) {
            if(field3Matrix(i,j).x != field3MatrixRead(i,j).x) throw std::runtime_error("field3Matrix.x != field3MatrixRead.x at elem: " + std::to_string(i) +" "+ std::to_string(j));
            if(field3Matrix(i,j).y != field3MatrixRead(i,j).y) throw std::runtime_error("field3Matrix.y != field3MatrixRead.y at elem: " + std::to_string(i) +" "+ std::to_string(j));
            if(field3Matrix(i,j).z != field3MatrixRead(i,j).z) throw std::runtime_error("field3Matrix.z != field3MatrixRead.z at elem: " + std::to_string(i) +" "+ std::to_string(j));
        }
    }



    return 0;
}