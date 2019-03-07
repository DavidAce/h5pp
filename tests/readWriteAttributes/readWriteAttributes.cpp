
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

    std::string stringDummy = "Dummy string with spaces";


    std::string output_filename         = "readWriteAttributes.h5";
    std::string output_folder           = "output";
    bool        create_dir_if_not_found = true;
    bool        overwrite_file_if_found = true;


    h5pp::File file("readTest.h5", "output",h5pp::AccessMode::RENAME,true,0);
    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;
    std::cout << "Writing vectorComplex     : \n" <<  vectorComplex  << std::endl;
    std::cout << "Writing stringDummy       : \n" <<  stringDummy    << std::endl;

    file.write_dataset(vectorDouble,"vectorDouble");
    file.write_dataset(vectorComplex,"vectorComplex");
    file.write_dataset(stringDummy,"stringDummy");
    file.write_attribute_to_link(std::string("This is an attribute"), "TestAttr", "vectorDouble"  );
    file.write_attribute_to_link(std::string("This is an attribute"), "TestAttr", "vectorComplex" );
    file.write_attribute_to_link("This is an attribute", "TestAttr", "stringDummy"   );


    // Read the data back
    std::vector<double>     vectorDoubleRead;
    std::vector<cplx>       vectorComplexRead;
    std::string             stringDummyRead;


    file.read_dataset(vectorDoubleRead,"vectorDouble");
    file.read_dataset(vectorComplexRead,"vectorComplex");
    file.read_dataset(stringDummyRead,"stringDummy");
    std::cout << "Reading vectorDouble: " <<  vectorDoubleRead  << std::endl;
    std::cout << "Reading vectorComplex: " <<  vectorComplexRead  << std::endl;
    std::cout << "Reading stringDummy: " <<  stringDummyRead  << std::endl;

    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}
    if (vectorComplex != vectorComplexRead){throw std::runtime_error("vectorComplex != vectorComplexRead");}
    if (stringDummy != stringDummyRead){throw std::runtime_error("stringDummy != stringDummyRead");}

    return 0;
}