//
// Created by david on 2019-09-03.
//

#ifndef H5PP_H5PPCONSTANTS_H
#define H5PP_H5PPCONSTANTS_H


namespace h5pp{
    namespace Constants{
        static constexpr unsigned long max_size_compact     = 32  * 1024;   //Max size of compact datasets is 32 kb
        static constexpr unsigned long max_size_contiguous  = 512 * 1024;   //Max size of contiguous datasets is 512 kb
    }
}
#endif //H5PP_H5PPCONSTANTS_H


