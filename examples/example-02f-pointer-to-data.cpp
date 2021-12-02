
#include <h5pp/h5pp.h>

/* In this example we consider passing a pointer to a data buffer
 *  It does not matter how the pointer is created, for instance any
 *  of these will do:
 *
 *       double array [10];
 *       double * array
 *       double * array = static_cast<double*>(malloc(sizeof(double) * 10));
 *       double * array = new double[10];
 *       std::vector<double> array(10); array.data()
 *
 *  In all but the first example the data dimension (or shape) information is lost,
 *  and therefore we must give it to h5pp manually.
 *
 *
 *  Here we will write and read a 1-dimensional array of size 10.
 *  Naturally, passing size > 10 will cause problems, but size < 10
 *  is fine.
 *  Also, it is possible to pass for instance {2,5} to reinterpret
 *  the buffer as a 2x5 matrix. See example 08a for more details.
 */

int main() {
    // Initialize a file
    h5pp::File file("exampledir/example-02f-pointer-to-data.h5", h5pp::FileAccess::REPLACE);

    // Initialize a dummy array buffer for writing
    auto *w_array = new double[10];
    for(size_t i = 0; i < 10; i++) w_array[i] = 3.14;

    // Write data
    file.writeDataset(w_array, "myArrayDouble", 10); // Size information can be a scalar "10" or a list "{10}"

    // Initialize a dummy array buffer for reading
    auto *r_array = new double[10];

    // Read data.
    // Note that h5pp will only resize containers with a ".resize()" member,
    // and therefore does NOT resize pointer buffers
    file.readDataset(r_array, "myArrayDouble", 10); // Size information can be a scalar "10" or a list "{10}"

    // Print
    h5pp::print("Read dataset: \n");
    for(size_t i = 0; i < 10; i++) h5pp::print("{}\n", r_array[i]);

    delete[] w_array;
    delete[] r_array;

    return 0;
}