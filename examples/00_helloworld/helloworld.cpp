
#include <h5pp/h5pp.h>
int main(){
    std::string myString     = "helloworld";
    std::string filename     = "helloh5pp.h5";
    std::string outputDir    = "output";
    std::string dataset_name = "helloworld";
    h5pp::File file(filename, outputDir);
    file.write_dataset(myString,dataset_name);
    std::cout << "Wrote string to hdf5 file: output/helloh5pp.h5" << std::endl;
    return 0;
}
