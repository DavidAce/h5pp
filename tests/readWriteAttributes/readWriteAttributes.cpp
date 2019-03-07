
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


int main()
{
    using cplx = std::complex<double>;

    static_assert(h5pp::Type::Check::has_member_data<std::vector<double>>() and "Compile time type-checker failed. Could not properly detect class member data. Check that you are using a supported compiler!");

    // Generate dummy data
    std::vector<double> vectorDouble = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                    -1.0, 0.0, 1.0, 0.0,-1.0, 0.0, 0.0,-1.0, 0.0, 1.0, 0.0, 1.0};



    std::string outputFilename      = "readWriteAttributes.h5";
    std::string outputDir           = "output";
    bool        createDir = true;
    size_t      logLevel  = 0;
    h5pp::File file(outputFilename, outputDir,h5pp::AccessMode::TRUNCATE,createDir,logLevel);


    std::cout << "Writing vectorDouble      : \n" <<  vectorDouble   << std::endl;

    file.write_dataset(vectorDouble,"testGroup/vectorDouble");

    file.write_attribute_to_link(std::string("This is a string"), "AttributeString"           , "testGroup/vectorDouble"  );
    file.write_attribute_to_link("This is a char array"         , "AttributeCharArray"        , "testGroup/vectorDouble"  );
    file.write_attribute_to_link(1                              , "AttributeInt"              , "testGroup/vectorDouble"  );
    file.write_attribute_to_link(1.0                            , "AttributeDouble"           , "testGroup/vectorDouble"  );
    file.write_attribute_to_link(std::complex<double>(2,3)      , "AttributeComplexDouble"    , "testGroup/vectorDouble"  );
    file.write_attribute_to_link((int[]){1,2,3,4,5,6}           , "AttributeIntArray"         , "testGroup/vectorDouble"  );
    file.write_attribute_to_link(vectorDouble                   , "AttributeVectorDouble"     , "testGroup/vectorDouble"  );


    // Read the data back
    std::vector<double>     vectorDoubleRead;
    std::cout << "Reading testGroup/vectorDouble: " <<  vectorDoubleRead  << std::endl;
    file.read_dataset(vectorDoubleRead,"testGroup/vectorDouble");
    if (vectorDouble != vectorDoubleRead){throw std::runtime_error("vectorDouble != vectorDoubleRead");}

    return 0;
}