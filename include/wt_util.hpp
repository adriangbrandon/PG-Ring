//
// Created by adrian on 2/10/25.
//

#ifndef WT_UTIL_HPP
#define WT_UTIL_HPP

#include <sdsl/wm_int.hpp>
#include <sdsl/wt_helper.hpp>

namespace wt_util {

    using value_type =  typename sdsl::wm_int<>::value_type;
    using node_type  =  typename sdsl::wm_int<>::node_type;


    private:

    public:

    template<class wt_t>
    std::vector<value_type> values_2dranges(const wt_t* wt_ptr, const sdsl::range_type &range,
                                           const std::vector<sdsl::range_type> &sigma_ranges) {



   }

}

#endif //WT_UTIL_HPP
