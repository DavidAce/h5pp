
// Set the default compression level
// Note 1: h5pp uses HDF5's zlib filter to compress data.
// Note 2: The compression level is a number 0-9, where 0 is no compression and 9 is max compression.
// Note 3: Highest compression levels require considerable computational effort, often without added benefit.
//         For good compression/performance ratio consider levels in the range 2-5.
file.setCompressionLevel(4);


// Write data.
// Note 1: Compression only applies to datasets with H5D_CHUNKED layout. The compression filter
//         is applied to each chunk.
// Note 1
file.writeDataset(v_write, "myStdVectorDouble",H5D_CHUNKED);