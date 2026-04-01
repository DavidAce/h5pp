#include <h5pp/h5pp.h>
#include <vector>

/* In this example we consider passing a pointer to a data buffer.
 * It does not matter how the pointer is created, for instance any
 * of these will do:
 *
 *      double array[10];
 *      double *array;
 *      double *array = new double[10];
 *      std::vector<double> array(10); array.data();
 *
 * In all but the first example the data dimension (or shape) information is lost,
 * and therefore we must give it to h5pp manually.
 *
 * Here we will write and read a 1-dimensional array of size 10.
 * Naturally, passing size > 10 will cause problems, but size < 10
 * is fine.
 * Also, it is possible to pass for instance {2,5} to reinterpret
 * the buffer as a 2x5 matrix. See example 08a for more details.
 */

void printBuffer(std::string_view message, const double *buffer, size_t size) {
    h5pp::print("{}\n", message);
    for(size_t idx = 0; idx < size; idx++) h5pp::print("{}\n", buffer[idx]);
}

void longForm(h5pp::File &file) {
    // Initialize a dummy array buffer for writing.
    std::vector<double> writeBuffer(10, 3.14);
    auto               *writePtr = writeBuffer.data();

    h5pp::DatasetWriteOptions writeOptions;
    writeOptions.dims = {writeBuffer.size()}; // Size information can be a scalar "10" or a list "{10}"

    // Write data using the long-form dataset handle.
    file.dataset("longForm/myArrayDouble").write(writePtr, writeOptions);

    // Initialize a dummy array buffer for reading.
    std::vector<double> readBuffer(10, 0.0);
    auto               *readPtr = readBuffer.data();

    h5pp::DatasetReadOptions readOptions;
    readOptions.dims = {readBuffer.size()};

    // Read data using the long-form dataset handle.
    // Note that h5pp will only resize containers with a ".resize()" member,
    // and therefore does not resize pointer buffers.
    file.dataset("longForm/myArrayDouble").readInto(readPtr, readOptions);
    printBuffer("Long form read dataset:", readPtr, readBuffer.size());
}

void shortForm(h5pp::File &file) {
    // Initialize a dummy array buffer for writing.
    std::vector<double> writeBuffer(10, 3.14);
    auto               *writePtr = writeBuffer.data();

    h5pp::DatasetWriteOptions writeOptions;
    writeOptions.dims = {writeBuffer.size()}; // Size information can be a scalar "10" or a list "{10}"

    // Write data using the explicit File short form.
    file.writeDataset(writePtr, "shortForm/myArrayDouble", writeOptions);

    // Initialize a dummy array buffer for reading.
    std::vector<double> readBuffer(10, 0.0);
    auto               *readPtr = readBuffer.data();

    h5pp::DatasetReadOptions readOptions;
    readOptions.dims = {readBuffer.size()};

    // Read data using the explicit File short form.
    file.readDataset(readPtr, "shortForm/myArrayDouble", readOptions);
    printBuffer("Short form read dataset:", readPtr, readBuffer.size());
}

int main() {
    // Initialize a file
    h5pp::File file(H5PP_EXAMPLE_DIR "example-02f-pointer-to-data.h5", h5pp::FileAccess::REPLACE);
    longForm(file);  // Show the canonical handle-based API
    shortForm(file); // Show the compatibility short form
    return 0;
}
