//
// Created by Adrián on 16/3/23.
//
#include <sdsl/wavelet_trees.hpp>
#include <wt_range_iterator.hpp>
#include <wt_2dranges_iterator.hpp>


int main(int argc, char* argv[]) {
    sdsl::wm_int<> wm;
    //sdsl::int_vector<> vec = {2, 3, 6 , 8, 2, 1, 2, 3, 4, 5 ,6, 3 ,4, 3, 5, 8};
    sdsl::int_vector<> vec = {0, 2, 3, 6, 7, 2, 1, 2, 3, 4, 5 ,6, 3 ,4, 3, 5, 7};
    sdsl::construct_im(wm , vec);

    sdsl::wt_range_iterator<sdsl::wm_int<>> iterator(&wm, sdsl::range_type{3, 9});

    auto v = iterator.next();
    while(v != 0){
        std::cout << v << std::endl;
        v = iterator.next();
    }

    std::cout << std::endl;
    std::cout << "2D Ranges:" << std::endl;
    //std::vector<sdsl::range_type> sigma_ranges = {sdsl::range_type{1,2}, sdsl::range_type{4,4}, sdsl::range_type{7,8}};
    std::vector<sdsl::range_type> sigma_ranges = { sdsl::range_type{4,4}, sdsl::range_type{7,8}};
    sdsl::wt_2dranges_iterator<sdsl::wm_int<>> iterator_2d(&wm, sdsl::range_type{2, 8}, sigma_ranges);

    // wm.select_next()

    auto a =  wm.select_next(3, sigma_ranges);
    std::cout << "Select next of 3: " << a << std::endl;

    std::vector<sdsl::range_type> ranges = {sdsl::range_type{0,0}, sdsl::range_type{4,6}};
    auto b = wm.range_next_value(3, ranges);
    std::cout << "Next value: " << b << std::endl;

    v = iterator_2d.next();
    while(v != 0){
        std::cout << v << std::endl;
        v = iterator_2d.next();
    }


    std::cout << std::endl;
    auto res = wm.range2d_values(sdsl::range_type{2,8}, sigma_ranges);
    for (const auto& v : res) {
        std::cout << v << std::endl;
    }

    sigma_ranges = {sdsl::range_type{5,6}};
    auto p = wm.select_next_pos_with_value(4, sigma_ranges);
    std::cout << "Select next pos with value 4: " << p.first << ", " << p.second << std::endl;

    p = wm.select_next_pos_with_value(2, sigma_ranges);
    std::cout << "Select next pos with value 2: " << p.first << ", " << p.second << std::endl;

    sigma_ranges = {sdsl::range_type{1,1}, sdsl::range_type{3,7}};
    p = wm.select_next_pos_with_value(0, sigma_ranges);
    std::cout << "Select next pos with value 0: " << p.first << ", " << p.second << std::endl;

    sigma_ranges = {sdsl::range_type{6,6}};
    a =  wm.select_next(4, sigma_ranges);
    std::cout << "Select next of 3: " << a << std::endl;

    ranges = { sdsl::range_type{1,2}, sdsl::range_type{4,13}};
    a = wm.select_next_ranges(ranges, 2);
    std::cout << "Select next of 6 in ranges: " << a << std::endl;

}
