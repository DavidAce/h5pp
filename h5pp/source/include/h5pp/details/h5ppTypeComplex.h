//
// Created by david on 2019-03-04.
//

#ifndef H5PP_TYPECOMPLEX_H
#define H5PP_TYPECOMPLEX_H

//#include <h5pp/details/h5ppType.h>

namespace h5pp{
    namespace Type{
        namespace Complex{
            inline hid_t H5T_COMPLEX_INT;
            inline hid_t H5T_COMPLEX_LONG;
            inline hid_t H5T_COMPLEX_UINT;
            inline hid_t H5T_COMPLEX_ULONG;
            inline hid_t H5T_COMPLEX_DOUBLE;
            inline hid_t H5T_COMPLEX_FLOAT;
            template <typename T>
            struct H5T_COMPLEX_STRUCT {
                T real;   /*real part*/
                T imag;   /*imaginary part*/
            };

        }

    }

}



#endif //H5PP_H5PPTYPECOMPLEX_H
