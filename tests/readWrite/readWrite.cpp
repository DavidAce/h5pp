
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

    static_assert(h5pp::Type::Check::has_member_data<std::vector<double>>() and "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    // Generate dummy data
    std::vector<double> vectorDouble = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                    -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};
    std::vector<cplx> vectorComplex = {-0.191154 + 0.326211i, 0.964728-0.712335i, -0.0351791-0.10264i,0.177544+0.99999i};

    Eigen::Tensor<cplx,4> tensorComplex(3,3,3,3);
    tensorComplex.setRandom();
    std::string stringDummy = "Dummy string with spaces";


    std::string output_filename         = "readTest.h5";
    std::string output_folder           = "output";
    bool        create_dir_if_not_found = true;
    bool        overwrite_file_if_found = true;


    h5pp::File file("readTest.h5", "output",h5pp::AccessMode::RENAME,true,0);
    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;
    std::cout << "Writing vectorComplex     : \n" <<  vectorComplex  << std::endl;
    std::cout << "Writing tensorComplex     : \n" <<  tensorComplex  << std::endl;
    std::cout << "Writing stringDummy       : \n" <<  stringDummy    << std::endl;

    file.write_dataset(vectorDouble,"vectorDouble");
    file.write_dataset(vectorComplex,"vectorComplex");
    file.write_dataset(tensorComplex,"tensorComplex");
    file.write_dataset(stringDummy,"stringDummy");
    file.write_attribute_to_dataset("vectorDouble"   ,std::string("This is an attribute"), "TestAttr");
    file.write_attribute_to_dataset("vectorComplex"  ,std::string("This is an attribute"), "TestAttr");
    file.write_attribute_to_dataset("tensorComplex"  ,std::string("This is an attribute"), "TestAttr");
    file.write_attribute_to_dataset("stringDummy"    ,std::string("This is an attribute"), "TestAttr");


    // Read the data back
    std::vector<double>     vectorDoubleRead;
    std::vector<cplx>       vectorComplexRead;
    Eigen::Tensor<cplx,4>   tensorComplexRead;
    std::string             stringDummyRead;


    file.read_dataset(vectorDoubleRead,"vectorDouble");
    file.read_dataset(vectorComplexRead,"vectorComplex");
    file.read_dataset(tensorComplexRead,"tensorComplex");
    file.read_dataset(stringDummyRead,"stringDummy");
    std::cout << "Reading vectorDouble: " <<  vectorDoubleRead  << std::endl;
    std::cout << "Reading vectorComplex: " <<  vectorComplexRead  << std::endl;
    std::cout << "Reading tensorComplex: " <<  tensorComplexRead  << std::endl;
    std::cout << "Reading stringDummy: " <<  stringDummyRead  << std::endl;

    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}
    if (vectorComplex != vectorComplexRead){throw std::runtime_error("vectorComplex != vectorComplexRead");}
    if (stringDummy != stringDummyRead){throw std::runtime_error("stringDummy != stringDummyRead");}

    //Tensor comparison isn't as straightforward
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