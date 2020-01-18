
#include <h5pp/details/h5ppRawArrayWrapper.h>
#include <h5pp/details/h5ppTypeScan.h>
#include <iostream>

int main() {
    double arr1[8];
    double arr2[27];
    int    dims1[] = {2, 2, 2};
    int    dims2[] = {3, 3, 3};
    for(int i = 0; i < 8; i++) arr1[i] = i + 0.8;
    for(int i = 0; i < 27; i++) arr2[i] = i + 0.27;

    auto wrapped_arr1 = h5pp::Wrapper::RawArrayWrapper(arr1, dims1);
    auto wrapped_arr2 = h5pp::Wrapper::RawArrayWrapper(arr2, dims2);
    auto wrapped_arr3 = h5pp::Wrapper::RawArrayWrapper(arr2, {3, 3, 3});

    for(auto &elem : wrapped_arr1) { std::cout << elem << std::endl; }

    for(auto &elem : wrapped_arr2) { std::cout << elem << std::endl; }
    for(auto &elem : wrapped_arr3) { std::cout << elem << std::endl; }

    std::array<int, 2>  dims         = {1, 2};
    std::vector<double> data         = {1, 2, 3, 4};
    auto                wrapped_arr4 = h5pp::Wrapper::RawArrayWrapper(data.data(), dims);

    for(auto &elem : wrapped_arr4) { std::cout << elem << std::endl; }

    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4)>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4)::value_type>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.data())>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.size())>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.begin())>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.end())>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.front())>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.back())>() << std::endl;
    std::cout << h5pp::Type::Scan::type_name<decltype(wrapped_arr4.dimensions())>() << std::endl;

    return 0;
}
