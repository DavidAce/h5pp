#pragma once

#include "h5ppHid.h"
#include "h5ppTypeCompound.h"
#include "h5ppTypeScan.h"
#include <complex>

namespace h5pp::Type::Compound::Create {

    template<typename T>
    [[nodiscard]] Hid::h5t createComplexType() {
        Hid::h5t NEW_COMPLEX_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(H5T_COMPLEX_STRUCT<T>));
        Hid::h5t h5type           = h5pp::Type::Scan::getH5DataType<T>();
        herr_t   errr             = H5Tinsert(NEW_COMPLEX_TYPE, "real", HOFFSET(H5T_COMPLEX_STRUCT<T>, real), h5type);
        herr_t   erri             = H5Tinsert(NEW_COMPLEX_TYPE, "imag", HOFFSET(H5T_COMPLEX_STRUCT<T>, imag), h5type);
        if(errr < 0 or erri < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to insert x or y in new complex type: \n real " + std::to_string(errr) + "\n imag " + std::to_string(erri));
        }
        return H5Tcopy(NEW_COMPLEX_TYPE);
    }

    template<typename T>
    [[nodiscard]] Hid::h5t createScalar2Type() {
        Hid::h5t NEW_SCALAR2_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(H5T_SCALAR2<T>));
        Hid::h5t h5type           = h5pp::Type::Scan::getH5DataType<T>();
        herr_t   errx             = H5Tinsert(NEW_SCALAR2_TYPE, "x", HOFFSET(H5T_SCALAR2<T>, x), h5type);
        herr_t   erry             = H5Tinsert(NEW_SCALAR2_TYPE, "y", HOFFSET(H5T_SCALAR2<T>, y), h5type);
        if(errx < 0 or erry < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to insert x or y in new Scalar2: \n x " + std::to_string(errx) + "\n y " + std::to_string(erry));
        }
        return NEW_SCALAR2_TYPE;
    }

    template<typename T>
    [[nodiscard]] Hid::h5t createScalar3Type() {
        Hid::h5t NEW_SCALAR3_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(H5T_SCALAR3<T>));
        Hid::h5t h5type           = h5pp::Type::Scan::getH5DataType<T>();
        herr_t   errx, erry, errz;
        errx = H5Tinsert(NEW_SCALAR3_TYPE, "x", HOFFSET(H5T_SCALAR3<T>, x), h5type);
        erry = H5Tinsert(NEW_SCALAR3_TYPE, "y", HOFFSET(H5T_SCALAR3<T>, y), h5type);
        errz = H5Tinsert(NEW_SCALAR3_TYPE, "z", HOFFSET(H5T_SCALAR3<T>, z), h5type);
        if(errx < 0 or erry < 0 or errz < 0) {
            H5Eprint(H5E_DEFAULT, stderr);
            throw std::runtime_error("Failed to insert x y or z in new Scalar3: \n x " + std::to_string(errx) + "\n y " + std::to_string(erry) + "\n z" + std::to_string(errz));
        }

        return NEW_SCALAR3_TYPE;
    }

}

namespace h5pp::Type::Compound {

    inline void initTypes() {
        H5T_COMPLEX_INT    = Create::createComplexType<int>();
        H5T_COMPLEX_LONG   = Create::createComplexType<long>();
        H5T_COMPLEX_LLONG  = Create::createComplexType<long long>();
        H5T_COMPLEX_UINT   = Create::createComplexType<unsigned int>();
        H5T_COMPLEX_UINT   = Create::createComplexType<unsigned int>();
        H5T_COMPLEX_ULONG  = Create::createComplexType<unsigned long>();
        H5T_COMPLEX_ULLONG = Create::createComplexType<unsigned long long>();
        H5T_COMPLEX_DOUBLE = Create::createComplexType<double>();
        H5T_COMPLEX_FLOAT  = Create::createComplexType<float>();

        H5T_SCALAR2_INT    = Create::createScalar2Type<int>();
        H5T_SCALAR2_LONG   = Create::createScalar2Type<long>();
        H5T_SCALAR2_LLONG  = Create::createScalar2Type<long long>();
        H5T_SCALAR2_UINT   = Create::createScalar2Type<unsigned int>();
        H5T_SCALAR2_ULONG  = Create::createScalar2Type<unsigned long>();
        H5T_SCALAR2_ULLONG = Create::createScalar2Type<unsigned long long>();
        H5T_SCALAR2_DOUBLE = Create::createScalar2Type<double>();
        H5T_SCALAR2_FLOAT  = Create::createScalar2Type<float>();

        H5T_SCALAR3_INT    = Create::createScalar3Type<int>();
        H5T_SCALAR3_LONG   = Create::createScalar3Type<long>();
        H5T_SCALAR3_LLONG  = Create::createScalar3Type<long long>();
        H5T_SCALAR3_UINT   = Create::createScalar3Type<unsigned int>();
        H5T_SCALAR3_ULONG  = Create::createScalar3Type<unsigned long>();
        H5T_SCALAR3_ULLONG = Create::createScalar3Type<unsigned long long>();
        H5T_SCALAR3_DOUBLE = Create::createScalar3Type<double>();
        H5T_SCALAR3_FLOAT  = Create::createScalar3Type<float>();
    }

    inline void closeTypes() {
        H5Tclose(H5T_COMPLEX_INT);
        H5Tclose(H5T_COMPLEX_LONG);
        H5Tclose(H5T_COMPLEX_LLONG);
        H5Tclose(H5T_COMPLEX_UINT);
        H5Tclose(H5T_COMPLEX_ULONG);
        H5Tclose(H5T_COMPLEX_ULLONG);
        H5Tclose(H5T_COMPLEX_DOUBLE);
        H5Tclose(H5T_COMPLEX_FLOAT);

        H5Tclose(H5T_SCALAR2_INT);
        H5Tclose(H5T_SCALAR2_LONG);
        H5Tclose(H5T_SCALAR2_LLONG);
        H5Tclose(H5T_SCALAR2_UINT);
        H5Tclose(H5T_SCALAR2_ULONG);
        H5Tclose(H5T_SCALAR2_ULLONG);
        H5Tclose(H5T_SCALAR2_DOUBLE);
        H5Tclose(H5T_SCALAR2_FLOAT);

        H5Tclose(H5T_SCALAR3_INT);
        H5Tclose(H5T_SCALAR3_LONG);
        H5Tclose(H5T_SCALAR3_LLONG);
        H5Tclose(H5T_SCALAR3_UINT);
        H5Tclose(H5T_SCALAR3_ULONG);
        H5Tclose(H5T_SCALAR3_ULLONG);
        H5Tclose(H5T_SCALAR3_DOUBLE);
        H5Tclose(H5T_SCALAR3_FLOAT);
    }

}