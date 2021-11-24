#include <h5pp/h5pp.h>

/*
 * An attribute is similar to a dataset, but can be appended to other HDF5 objects like datasets, groups or files.
 * Users can append any number of attributes to a given HDF5 object. A common use case for attributes
 * is to add descriptive metadata to an HDF5 object.
 *
 * This example shows how to get all attributes from a link and print their contents
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-03b-attributes-traverse.h5", h5pp::FileAccess::REPLACE);

    // Write a dummy integer to file
    file.writeDataset(42, "intGroup/myInt");

    // Write some attributes to the dataset
    file.writeAttribute("this is some info about my int", "myInt_stringAttribute", "intGroup/myInt");
    file.writeAttribute(3.14, "myInt_doubleAttribute", "intGroup/myInt");
    file.writeAttribute(std::vector<int>{1, 2, 3, 4}, "myInt_vectorAttribute", "intGroup/myInt");

    // Now we imagine wanting to iterate through all attributes of "intGroup/myInt" and print their contents.
    // The most pressing concern is that we may not know beforehand what type we should read data into.
    // This has no simple solution, since c++ is statically typed. We have to check a list of types during runtime.

    // In this case we will use h5pp to obtain type metadata, so that we can use the correct type when calling
    //      file.readAttribute<type>(...)

    auto typeInfoAttributes = file.getTypeInfoAttributes("intGroup/myInt"); // Get a vector of structs "TypeInfo", one for each attribute.
    for(const auto &typeInfo : typeInfoAttributes) {
        if(typeInfo.cppTypeIndex) { // Check that cppTypeIndex has been set (it normally has)
            // cppTypeIndex.value() is an object of type std::type_index which can be compared against typeid(T).
            // Here we will simply test each type and print the name and contents of the attribute if there is a match

            // WARNING: The type in cppTypeIndex always refers to the innermost scalar type, or "atomic" type. E.g. an array of 4 ints on file
            //          will have cppTypeIndex.value() == typeid(int), and dimension 4. This allows h5pp to be agnostic
            //          to which container the data is read into.

            auto msg = h5pp::format("Attribute {}::{} [{} | size {}]",
                                    typeInfo.h5Path.value(),
                                    typeInfo.h5Name.value(),
                                    typeInfo.cppTypeName.value(),
                                    typeInfo.h5Size.value());

            if(typeInfo.cppTypeIndex.value() == typeid(std::string) and typeInfo.h5Size.value() == 1)
                h5pp::print("{} = {}\n", msg, file.readAttribute<std::string>(typeInfo.h5Name.value(), typeInfo.h5Path.value()));
            if(typeInfo.cppTypeIndex.value() == typeid(int) and typeInfo.h5Size.value() == 1)
                h5pp::print("{} = {}\n", msg, file.readAttribute<int>(typeInfo.h5Name.value(), typeInfo.h5Path.value()));
            if(typeInfo.cppTypeIndex.value() == typeid(int) and typeInfo.h5Size.value() > 1)
                h5pp::print("{} = {}\n", msg, file.readAttribute<std::vector<int>>(typeInfo.h5Name.value(), typeInfo.h5Path.value()));
            if(typeInfo.cppTypeIndex.value() == typeid(double) and typeInfo.h5Size.value() == 1)
                h5pp::print("{} = {}\n", msg, file.readAttribute<double>(typeInfo.h5Name.value(), typeInfo.h5Path.value()));
            if(typeInfo.cppTypeIndex.value() == typeid(double) and typeInfo.h5Size.value() > 1)
                h5pp::print("{} = {}\n", msg, file.readAttribute<std::vector<double>>(typeInfo.h5Name.value(), typeInfo.h5Path.value()));

            // ... and so on. In practice, it is simpler to create templated helper functions to avoid code duplication here.

        }
    }

    return 0;
}

/* Program output:

    Attribute /intGroup/myInt::myInt_stringAttribute [std::__cxx11::basic_string<char> | size 1] = this is some info about my int
    Attribute /intGroup/myInt::myInt_doubleAttribute [double | size 1] = 3.14
    Attribute /intGroup/myInt::myInt_vectorAttribute [int | size 4] = [1, 2, 3, 4]

 */