
#include <iostream>
#include <h5pp/h5pp.h>

int main(){

    h5pp::File file("someFile.h5", "output", true,false,true);
    using namespace std::complex_literals;
    std::vector<std::complex<double>> testdata (5, 10.0 + 5.0i);
    std::vector<std::complex<int>> testdata2 (5, std::complex<int>(8,5));
    Eigen::MatrixXcd testdata3 (2,2);


    testdata3 << 1.0 + 2.0i,  3.0 + 4.0i, 5.0+6.0i , 7.0+8.0i;
    file.write_dataset(testdata,"testdata");
    file.write_dataset(testdata2,"testdata2");
    file.write_dataset(testdata3,"testdata3");
    return 0;
}
