//
// Created by david on 2019-03-01.

#include <Eigen/Core>
#include <h5pp/details/h5ppFile.h>
#include <spdlog/spdlog.h>

//#include <include/general/h5ppFile.h>
//#include <include/h5pp/details/h5ppDataType.h>

namespace fs = std::experimental::filesystem;
namespace tc = h5pp::Type::Check;

h5pp::File::File(const std::string output_filename_, const std::string output_folder_, bool overwrite_, bool resume_, bool create_dir_) {
    h5pp::Logger::set_logger("h5pp",3,false);

    outputFilename = output_filename_;
    outputFolder = output_folder_;
    createDir = create_dir_;
    overwrite = overwrite_;
    resume = resume_;
    if (outputFolder.empty()) {
        if (outputFilename.is_relative()) {
            outputFolder = fs::current_path();
        } else if (outputFilename.is_absolute()) {
            outputFolder = outputFolder.stem();
            outputFilename = outputFilename.filename();
        }
    }


    /*
    * Check if zlib compression is available and can be used for both
    * compression and decompression.  Normally we do not perform error
    * checking in these examples for the sake of clarity, but in this
    * case we will make an exception because this filter is an
    * optional part of the hdf5 library.
    */
        herr_t          status;
        htri_t          avail;
        H5Z_filter_t    filter_type;
        unsigned int    flags, filter_info;
        avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
        if (!avail) {
            spdlog::warn("zlib filter not available");
        }
        status = H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filter_info);
        if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
             !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) ) {
            spdlog::warn("zlib filter not available for encoding and decoding");
        }

    initialize();

}




void h5pp::File::initialize(){
    set_output_file_path();
    plist_facc = H5Pcreate(H5P_FILE_ACCESS);
    plist_lncr = H5Pcreate(H5P_LINK_CREATE);   //Create missing intermediate group if they don't exist
    plist_xfer = H5Pcreate(H5P_DATASET_XFER);
    plist_lapl = H5Pcreate(H5P_LINK_ACCESS);
    H5Pset_create_intermediate_group(plist_lncr, 1);
    h5pp::Type::Complex::initTypes();
//    H5T_COMPLEX_DOUBLE = H5Tcreate (H5T_COMPOUND, sizeof(H5T_COMPLEX_STRUCT));
//    H5Tinsert (H5T_COMPLEX_DOUBLE, "real", HOFFSET(H5T_COMPLEX_STRUCT,real), H5T_NATIVE_DOUBLE);
//    H5Tinsert (H5T_COMPLEX_DOUBLE, "imag", HOFFSET(H5T_COMPLEX_STRUCT,imag), H5T_NATIVE_DOUBLE);

    switch (fileMode){
        case FileMode::OPEN: {spdlog::info("File mode OPEN: {}", outputFileFullPath.string()); break;}
        case FileMode::TRUNCATE: {
            spdlog::info("File mode TRUNCATE: {}", outputFileFullPath.string());
            try{
                hid_t file = H5Fcreate(outputFileFullPath.c_str(), H5F_ACC_TRUNC,  H5P_DEFAULT, plist_facc);
                H5Fclose(file);
            }catch(std::exception &ex){
                throw(std::runtime_error("Failed to create hdf5 file"));
            }
            //Put git revision in file attribute
//            std::string gitversion = "Git branch: " + GIT::BRANCH + " | Commit hash: " + GIT::COMMIT_HASH + " | Revision: " + GIT::REVISION;
//            write_attribute_to_file(gitversion, "GIT REVISION");
            break;
        }
        case FileMode::RENAME: {
            outputFileFullPath =  get_new_filename(outputFileFullPath);
            spdlog::info("Renamed output file: {} ---> {}", outputFilename.string(),outputFileFullPath.filename().string());
            spdlog::info("File mode RENAME: {}", outputFileFullPath.string());
            try{
                hid_t file = H5Fcreate(outputFileFullPath.c_str(), H5F_ACC_TRUNC,  H5P_DEFAULT, plist_facc);
                H5Fclose(file);
            }catch(std::exception &ex){
                throw(std::runtime_error("Failed to create hdf5 file"));
            }
            //Put git revision in file attribute
//            std::string gitversion = "Git branch: " + GIT::BRANCH + " | Commit hash: " + GIT::COMMIT_HASH + " | Revision: " + GIT::REVISION;
//            write_attribute_to_file(gitversion, "GIT REVISION");
            break;
        }
        default:{
            spdlog::error("File Mode not set. Choose |OPEN|TRUNCATE|RENAME|");
            throw std::runtime_error("File Mode not set. Choose |OPEN|TRUNCATE|RENAME|");
        }
    }
}

bool h5pp::File::file_is_valid(){
    return file_is_valid(outputFileFullPath);
}

bool h5pp::File::file_is_valid(fs::path some_hdf5_filename) {
    if (fs::exists(some_hdf5_filename)){
        if (H5Fis_hdf5(outputFileFullPath.c_str()) > 0) {
            return true;
        } else {
            return false;
        }
    }else{
        return false;
    }
}

fs::path h5pp::File::get_new_filename(fs::path some_hdf5_filename){
    int i=1;
    fs::path some_new_hdf5_filename = some_hdf5_filename;
    while (fs::exists(some_new_hdf5_filename)){
        some_new_hdf5_filename.replace_filename(some_hdf5_filename.stem().string() + "-" + std::to_string(i++) + some_hdf5_filename.extension().string() );
    }
    return some_new_hdf5_filename;
}

void h5pp::File::set_output_file_path() {
    fs::path current_folder    = fs::current_path();
    fs::path output_folder_abs = fs::system_complete(outputFolder);
    spdlog::debug("outputFolder     : {}", outputFolder.string() );
    spdlog::debug("output_folder_abs : {}", output_folder_abs.string() );
    spdlog::debug("outputFilename   : {}", outputFilename.string() );
    outputFileFullPath = fs::system_complete(output_folder_abs / outputFilename);
    if(file_is_valid(outputFileFullPath)){
        //If we are here, the file exists in the given folder. Then we have two options:
        // 1) If we can overwrite, we discard the old file and make a new one with the same name
        // 2) If we can't overwrite, we .
        outputFileFullPath = fs::canonical(outputFileFullPath);
        spdlog::info("File already exists: {}", outputFileFullPath.string());
        if(overwrite and not resume){
            spdlog::debug("Overwrite: true | Resume: false --> TRUNCATE");
            fileMode = FileMode::OPEN;
            return;
        }else if (overwrite and resume) {
            spdlog::debug("Overwrite: true | Resume: true --> OPEN");
            fileMode = FileMode::OPEN;
            return;
        }else if (not overwrite and not resume){
            spdlog::debug("Overwrite: false | Resume: false --> RENAME");
            fileMode = FileMode::RENAME;
            return;
        }else if (not overwrite and resume ){
            spdlog::debug("Overwrite: false | Resume: true --> OPEN");
            spdlog::error("Overwrite is off and resume is on - these are mutually exclusive settings. Defaulting to FileMode::OPEN");
            fileMode = FileMode::OPEN;
            return;
        }
    }else {
        // If we are here, the file does not exist in the given folder. Then we have two options:
        // 1 ) If the given folder exists, just use the current filename.
        // 2 ) If the given folder doesn't exist, check the option "createDir":
        //      a) Create dir and file in it
        //      b) Exit with an error
        fileMode = FileMode::TRUNCATE;
        if(fs::exists(output_folder_abs)){
            return;
        }else{
            if(createDir){
                if (fs::create_directories(output_folder_abs)){
                    output_folder_abs = fs::canonical(output_folder_abs);
                    spdlog::info("Created directory: {}",output_folder_abs.string());
                }else{
                    spdlog::info("Directory already exists: {}",output_folder_abs.string());
                }
            }else{
                spdlog::critical("Target folder does not exist and creation of directory is disabled in settings");
                exit(1);
            }
        }
    }


}



void h5pp::File::write_symbolic_link(const std::string &src_path, const std::string &tgt_path){
    hid_t file = open_file();
//    hid_t src_link = H5Dopen(file, src_path.c_str(), H5P_DEFAULT);
    bool exists = h5pp::Hdf5::check_if_link_exists_recursively(file, src_path);
    if (not exists)throw std::runtime_error("Trying to write soft link to non-existing path: " + src_path);
    retval = H5Lcreate_soft(src_path.c_str(), file, tgt_path.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    H5Fclose(file);
}





void h5pp::File::create_dataset_link(hid_t file, const DatasetProperties &props){
    if (not h5pp::Hdf5::check_if_link_exists_recursively(file, props.dset_name)){
        hid_t dataspace = h5pp::Utils::get_DataSpace_unlimited(props.ndims);
        hid_t dset_cpl  = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_layout(dset_cpl, H5D_CHUNKED);
        H5Pset_chunk(dset_cpl, props.ndims, props.chunk_size.data());
//        H5Pset_deflate (dset_cpl ,props.compression_level);
        hid_t dataset = H5Dcreate(file,
                                  props.dset_name.c_str(),
                                  props.datatype,
                                  dataspace,
                                  plist_lncr,
                                  dset_cpl,
                                  H5P_DEFAULT);
        H5Dclose(dataset);
        H5Sclose(dataspace);
        H5Pclose(dset_cpl);
    }
}

void h5pp::File::create_group_link(const std::string &group_relative_name) {
    //Check if group exists already
    hid_t file = open_file();
    spdlog::trace("Creating group link: {}", group_relative_name);
    std::stringstream path_stream(group_relative_name);
    std::vector<std::string> split_path;
    std::string head;
    std::string stem = "/";
    while(std::getline(path_stream, head, '/')){
        split_path.push_back(stem + head);
        stem += head + "/";
    }

    for(auto &group_name : split_path){
        if (not h5pp::Hdf5::check_if_link_exists_recursively(file, group_name)) {
            hid_t group = H5Gcreate(file, group_name.c_str(), plist_lncr, H5P_DEFAULT, H5P_DEFAULT);
            H5Gclose(group);
        }
    }
    H5Fclose(file);
}

void h5pp::File::select_hyperslab(const hid_t &filespace, const hid_t &memspace){
    const int ndims = H5Sget_simple_extent_ndims(filespace);
    std::vector<hsize_t> mem_dims(ndims);
    std::vector<hsize_t> file_dims(ndims);
    std::vector<hsize_t> start(ndims);
    H5Sget_simple_extent_dims(memspace , mem_dims.data(), nullptr);
    H5Sget_simple_extent_dims(filespace, file_dims.data(), nullptr);
    for(int i = 0; i < ndims; i++){
        start[i] = file_dims[i] - mem_dims[i];
    }
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start.data(), nullptr, mem_dims.data(), nullptr);
}




//herr_t
//file_info(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata)
//{
//    try{
////        hid_t group = H5Gopen2(loc_id, name, H5P_DEFAULT);
//        std::cout << "Name : " << name << std::endl; // Display the group name.
////        H5Gclose(group);
//
//    }catch(...){
//        throw(std::logic_error("Not a group: " + std::string(name)));
//    }
//    return 0;
//}
//std::vector<std::string> h5pp::File::print_contents_of_group(std::string group_name){
//    hid_t file = open_file();
//    spdlog::trace("Printing contents of group: {}",group_name);
//    try{
//        herr_t idx = H5Literate_by_name (file, group_name.c_str(), H5_INDEX_NAME,
//                                         H5_ITER_NATIVE, NULL, file_info, NULL,
//                                         H5P_DEFAULT);
//    }
//    catch(std::exception &ex){
//        spdlog::debug("Failed to get contents --  {}",ex.what());
//    }
//    H5Fclose(file);
//    return std::vector<std::string>();
//}
