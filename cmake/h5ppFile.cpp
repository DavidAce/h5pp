//
// Created by david on 2019-03-01.

//#include <Eigen/Core>
//#include <h5pp/details/h5ppFile.h>
//#include <spdlog/spdlog.h>

//#include <include/general/h5ppFile.h>
//#include <include/h5pp/details/h5ppDataType.h>

//namespace fs = std::experimental::filesystem;
//namespace tc = h5pp::Type::Check;




//
//void h5pp::File::initialize(){
//    h5pp::Logger::setLogger("h5pp-init",logLevel,false);
//    spdlog::debug("outputDir     : {}", outputDir.string() );
//    plist_facc = H5Pcreate(H5P_FILE_ACCESS);
//    plist_lncr = H5Pcreate(H5P_LINK_CREATE);   //Create missing intermediate group if they don't exist
//    plist_xfer = H5Pcreate(H5P_DATASET_XFER);
//    plist_lapl = H5Pcreate(H5P_LINK_ACCESS);
//    H5Pset_create_intermediate_group(plist_lncr, 1);
//    set_output_file_path();
//    h5pp::Type::Complex::initTypes();
//}
//
//
//
//void h5pp::File::set_output_file_path() {
//
//
//    if (outputDir.empty()) {
//        if (outputFilename.is_relative()) {
//            outputDir = fs::current_path();
//        } else if (outputFilename.is_absolute()) {
//            outputDir = outputDir.stem();
//            outputFilename = outputFilename.filename();
//        }
//    }
//
//
//    fs::path currentDir             = fs::current_path();
//    fs::path outputDirAbs           = fs::system_complete(outputDir);
//    spdlog::debug("currentDir        : {}", currentDir.string() );
//    spdlog::debug("outputDir         : {}", outputDir.string() );
//    spdlog::debug("outputDirAbs      : {}", outputDirAbs.string() );
//    spdlog::debug("outputFilename    : {}", outputFilename.string() );
//
//
//    if(createDir){
//        if (fs::create_directories(outputDirAbs)){
//            outputDirAbs = fs::canonical(outputDirAbs);
//            spdlog::info("Created directory: {}",outputDirAbs.string());
//        }else{
//            spdlog::info("Directory already exists: {}",outputDirAbs.string());
//        }
//    }else{
//        spdlog::critical("Target folder does not exist and creation of directory is disabled in settings");
//    }
//    outputFileFullPath = fs::system_complete(outputDirAbs / outputFilename);
//
//
//    switch (accessMode){
//        case AccessMode::OPEN: {
//            spdlog::debug("File mode OPEN: {}", outputFileFullPath.string());
//            try{
//                if(file_is_valid(outputFileFullPath)){
//                    hid_t file = open_file();
//                    close_file(file);
//                }
//            }catch(std::exception &ex){
//                throw std::runtime_error("Failed to open hdf5 file :" + outputFileFullPath.string() );
//
//            }
//            break;
//        }
//        case AccessMode::TRUNCATE: {
//            spdlog::debug("File mode TRUNCATE: {}", outputFileFullPath.string());
//            try{
//                hid_t file = H5Fcreate(outputFileFullPath.c_str(), H5F_ACC_TRUNC,  H5P_DEFAULT, plist_facc);
//                H5Fclose(file);
//                file = open_file();
//                herr_t close_file(file);
//            }catch(std::exception &ex){
//                throw std::runtime_error("Failed to create hdf5 file :" + outputFileFullPath.string() );
//            }
//            break;
//        }
//        case AccessMode::RENAME: {
//            try{
//                spdlog::debug("File mode RENAME: {}", outputFileFullPath.string());
//                if(file_is_valid(outputFileFullPath)) {
//                    outputFileFullPath =  get_new_filename(outputFileFullPath);
//                    spdlog::info("Renamed output file: {} ---> {}", outputFilename.string(),outputFileFullPath.filename().string());
//                    spdlog::info("File mode RENAME: {}", outputFileFullPath.string());
//                }
//                hid_t file = H5Fcreate(outputFileFullPath.c_str(), H5F_ACC_TRUNC,  H5P_DEFAULT, plist_facc);
//                H5Fclose(file);
//                file = open_file();
//                close_file(file);
//            }catch(std::exception &ex){
//                throw std::runtime_error("Failed to create renamed hdf5 file :" + outputFileFullPath.string() );
//            }
//            break;
//        }
//        default:{
//            spdlog::error("File Mode not set. Choose  AccessMode:: |OPEN|TRUNCATE|RENAME|");
//            throw std::runtime_error("File Mode not set. Choose  AccessMode::  |OPEN|TRUNCATE|RENAME|");
//        }
//    }
//
//
//
//
////
////
////
////    if(file_is_valid(outputFileFullPath)){
////        //If we are here, the file exists in the given folder. Then we have two options:
////        // 1) If we can overwrite, we discard the old file and make a new one with the same name
////        // 2) If we can't overwrite, we .
////        outputFileFullPath = fs::canonical(outputFileFullPath);
////        spdlog::info("File already exists: {}", outputFileFullPath.string());
////        if(overwrite and not resume){
////            spdlog::debug("Overwrite: true | Resume: false --> TRUNCATE");
////            accessMode = accessMode::OPEN;
////            return;
////        }else if (overwrite and resume) {
////            spdlog::debug("Overwrite: true | Resume: true --> OPEN");
////            accessMode = accessMode::OPEN;
////            return;
////        }else if (not overwrite and not resume){
////            spdlog::debug("Overwrite: false | Resume: false --> RENAME");
////            accessMode = accessMode::RENAME;
////            return;
////        }else if (not overwrite and resume ){
////            spdlog::debug("Overwrite: false | Resume: true --> OPEN");
////            spdlog::error("Overwrite is off and resume is on - these are mutually exclusive settings. Defaulting to accessMode::OPEN");
////            accessMode = accessMode::OPEN;
////            return;
////        }
////    }else {
////        // If we are here, the file does not exist in the given folder. Then we have two options:
////        // 1 ) If the given folder exists, just use the current filename.
////        // 2 ) If the given folder doesn't exist, check the option "createDir":
////        //      a) Create dir and file in it
////        //      b) Exit with an error
////        accessMode = accessMode::TRUNCATE;
////        if(fs::exists(outputDirAbs)){
////            return;
////        }else{
////            if(createDir){
////                if (fs::create_directories(outputDirAbs)){
////                    outputDirAbs = fs::canonical(outputDirAbs);
////                    spdlog::info("Created directory: {}",outputDirAbs.string());
////                }else{
////                    spdlog::info("Directory already exists: {}",outputDirAbs.string());
////                }
////            }else{
////                spdlog::critical("Target folder does not exist and creation of directory is disabled in settings");
////                exit(1);
////            }
////        }
////    }
//
//
//}


//void h5pp::File::set_output_file_path() {
//    fs::path current_folder    = fs::current_path();
//    fs::path output_folder_abs = fs::system_complete(outputDir);
//    spdlog::debug("outputDir     : {}", outputDir.string() );
//    spdlog::debug("output_folder_abs : {}", output_folder_abs.string() );
//    spdlog::debug("outputFilename   : {}", outputFilename.string() );
//    outputFileFullPath = fs::system_complete(output_folder_abs / outputFilename);
//    if(file_is_valid(outputFileFullPath)){
//        //If we are here, the file exists in the given folder. Then we have two options:
//        // 1) If we can overwrite, we discard the old file and make a new one with the same name
//        // 2) If we can't overwrite, we .
//        outputFileFullPath = fs::canonical(outputFileFullPath);
//        spdlog::info("File already exists: {}", outputFileFullPath.string());
//        if(overwrite and not resume){
//            spdlog::debug("Overwrite: true | Resume: false --> TRUNCATE");
//            accessMode = accessMode::OPEN;
//            return;
//        }else if (overwrite and resume) {
//            spdlog::debug("Overwrite: true | Resume: true --> OPEN");
//            accessMode = accessMode::OPEN;
//            return;
//        }else if (not overwrite and not resume){
//            spdlog::debug("Overwrite: false | Resume: false --> RENAME");
//            accessMode = accessMode::RENAME;
//            return;
//        }else if (not overwrite and resume ){
//            spdlog::debug("Overwrite: false | Resume: true --> OPEN");
//            spdlog::error("Overwrite is off and resume is on - these are mutually exclusive settings. Defaulting to accessMode::OPEN");
//            accessMode = accessMode::OPEN;
//            return;
//        }
//    }else {
//        // If we are here, the file does not exist in the given folder. Then we have two options:
//        // 1 ) If the given folder exists, just use the current filename.
//        // 2 ) If the given folder doesn't exist, check the option "createDir":
//        //      a) Create dir and file in it
//        //      b) Exit with an error
//        accessMode = accessMode::TRUNCATE;
//        if(fs::exists(output_folder_abs)){
//            return;
//        }else{
//            if(createDir){
//                if (fs::create_directories(output_folder_abs)){
//                    output_folder_abs = fs::canonical(output_folder_abs);
//                    spdlog::info("Created directory: {}",output_folder_abs.string());
//                }else{
//                    spdlog::info("Directory already exists: {}",output_folder_abs.string());
//                }
//            }else{
//                spdlog::critical("Target folder does not exist and creation of directory is disabled in settings");
//                exit(1);
//            }
//        }
//    }
//
//
//}



