#pragma once

#include "h5ppExcept.h"
#include "h5ppFloat128.h"
#include "h5ppFormat.h"
#include "h5ppHid.h"
#include "h5ppTypeSfinae.h"
#include <complex>
#include <H5Tpublic.h>

namespace h5pp::type::custom {

    class H5T_FLOAT128 {
        private:
        inline static std::string float_name;
        inline static hid::h5t    float_id;
        static void               init() {
            if(not float_id.valid()) {
#if __BYTE_ORDER == LITTLE_ENDIAN
                float_id   = H5Tcopy(H5T_IEEE_F64LE);
                float_name = "H5T_IEEE_F128LE";
#else
                float_id   = H5Tcopy(H5T_IEEE_F64BE);
                float_name = "H5T_IEEE_F128BE";
#endif
                if(H5Tset_size(float_id, 16) < 0) h5pp::runtime_error("H5Tset_size failed");
                if(H5Tset_precision(float_id, 128) < 0) h5pp::runtime_error("H5Tset_precision failed");
                if(H5Tset_offset(float_id, 0) < 0) h5pp::runtime_error("H5Tset_offset failed");
#if __BYTE_ORDER == LITTLE_ENDIAN
                if(H5Tset_fields(float_id, 127, 112, 15, 0, 112) < 0) h5pp::runtime_error("H5Tset_fields failed");
#else
                if(H5Tset_fields(float_id, 0, 1, 15, 16, 112) < 0) h5pp::runtime_error("H5Tset_fields failed");
#endif
                if(H5Tset_ebias(float_id, 127) < 0) throw h5pp::runtime_error("H5Tset_ebias failed");
                if(H5Tset_norm(float_id, H5T_NORM_MSBSET) < 0) throw h5pp::runtime_error("H5Tset_norm failed");
                if(H5Tset_inpad(float_id, H5T_PAD_ZERO) < 0) throw h5pp::runtime_error("H5Tset_inpad failed");
                if(H5Tset_pad(float_id, H5T_PAD_ZERO, H5T_PAD_ZERO) < 0) throw h5pp::runtime_error("H5Tset_pad failed");
            }
        }

        public:
        static hid::h5t &h5type() {
            init();
            return float_id;
        }

        static bool equal(const hid::h5t &other) {
            // Try to compare without initializing complex_id
            if(H5Tget_size(other) != 16) return false;
            init();
            if(float_id.valid() and H5Tequal(float_id, other) == 1) return true;
            return false;
        }
        static std::string &h5name() {
            init();
            return float_name;
        }
    };
}
