
#include <h5pp/h5pp.h>
int main(){
    std::string myString     = "helloworld";
    std::string filename     = "exampledir/helloworld.h5";
    std::string dataset_name = "helloworld";
    h5pp::File file(filename);
    file.writeDataset(myString, dataset_name);
    std::cout << "Wrote string to hdf5 file: output/helloh5pp.h5" << std::endl;
    return 0;
}
