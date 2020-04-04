#pragma once
//#include <H5Dpublic.h>
//#include <string>
//#include <iostream>
//namespace h5pp{
//
//
//    struct h5d_layout{
//        enum{
//            H5D_COMPACT = H5D_layout_t::H5D_COMPACT,
//            H5D_CONTIGUOUS = H5D_layout_t::H5D_CONTIGUOUS,
//            H5D_CHUNKED = H5D_layout_t::H5D_CHUNKED,
//        };
//
//        int val = h5d_layout::H5D_COMPACT;
//        operator int() const {return val;}
//        operator H5D_layout_t() const {return val;}
//        h5d_layout & operator=(const int & rhs){
//            val = rhs;
//            return *this;
//        }
//
//    };
//    void func(){
//        h5d_layout layout = 0;
//        switch(layout){
//            case(h5d_layout::H5D_COMPACT): std::cout << "compact" << std::endl;
//        }
//    }
//
//    struct h5d_layout{
//        private:
//        H5D_layout_t val = H5D_layout_t::H5D_COMPACT;
//        public:
//        constexpr static int H5D_COMPACT = H5D_layout_t::H5D_COMPACT;
//        constexpr static int H5D_CONTIGUOUS = H5D_layout_t::H5D_CONTIGUOUS;
//        constexpr static int H5D_CHUNKED = H5D_layout_t::H5D_CHUNKED;
//
//        h5d_layout(H5D_layout_t layout_):val(layout_){}
//
//
////        [[nodiscard]] operator H5D_layout_t() const { return value(); } // Class can be used as an actual H5D_layout_t
//        [[nodiscard]] const H5D_layout_t & value() const {
//            return val;
//        }
//        bool operator==(const h5d_layout & rhs) const {
//            return val == rhs.value();
//        }
//        bool operator==(const H5D_layout_t & rhs) const {
//            return val == rhs;
//        }
//        h5d_layout & operator=(const h5d_layout & rhs) {
//            val = rhs.value();
//            return *this;
//        }
//        explicit operator std::string() const {
//            if(val == H5D_COMPACT) return "H5D_COMPACT";
//            if(val == H5D_CONTIGUOUS) return "H5D_CONTIGUOUS";
//            if(val == H5D_CHUNKED) return "H5D_CHUNKED";
//            throw std::runtime_error("Invalid layout: " + std::to_string(val));
//        }
//        friend std::ostream &operator<<(std::ostream &os, const h5d_layout &l) { return os << l.val; }
//    };
//}
