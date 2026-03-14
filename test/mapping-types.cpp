//
// Created by adrian on 3/2/26.
//


#include <iostream>
#include <ostream>
#include <string>

#include "query/constant_utils.hpp"

int main() {
    std::string s = "-12.0";
    std::string t = "-2.0";
    std::string z = "0";
    std::string p = "10.0";
    std::int64_t res_s, res_t, res_z, res_p;
    bool ok_s = ring::query::constant::is_double(s, res_s);
    bool ok_t = ring::query::constant::is_double(t, res_t);
    bool ok_z = ring::query::constant::is_double(z, res_z);
    bool ok_p = ring::query::constant::is_double(p, res_p);
    std::cout << res_s << " " << res_t << " " << res_z << " " << res_p << " " << ok_t << " " << ok_z << std::endl;

    std::cout << ring::query::constant::int64_to_double(res_s) << std::endl;
    std::cout << ring::query::constant::int64_to_double(res_t) << std::endl;
    std::cout << ring::query::constant::int64_to_double(res_z) << std::endl;
    std::cout << ring::query::constant::int64_to_double(res_p) << std::endl;

    int64_t belgium = -1;
    std::cout << "-1: " << ring::query::constant::int64_to_double(belgium) << std::endl;

    std::string p1333lat = "49.49699";
    std::cout << p1333lat << std::endl;
    std::cout << ring::query::constant::is_double(p1333lat, belgium) << std::endl;
    std::cout << "Belgium: " << belgium << std::endl;
    std::cout << "Belgium: " << ring::query::constant::int64_to_double(belgium) << std::endl;
    uint64_t belgium_u;
    belgium_u = belgium - (-belgium-1);
    std::cout << "Belgium_u: " << belgium_u << std::endl;

    std::string test = "-2.5e10";
    int64_t r_test;
    std::cout << ring::query::constant::is_double(test, r_test) << std::endl;
    std::cout << r_test << std::endl;
    std::cout << ring::query::constant::int64_to_double(r_test) << std::endl;

    std::string test2 = "2.5e-10";
    int64_t r_test2;
    std::cout << ring::query::constant::is_double(test2, r_test2) << std::endl;
    std::cout << r_test2 << std::endl;
    std::cout << ring::query::constant::int64_to_double(r_test2) << std::endl;

    std::string test3 = "-2.5e-10";
    int64_t r_test3;
    std::cout << ring::query::constant::is_double(test3, r_test3) << std::endl;
    std::cout << r_test3 << std::endl;
    std::cout << ring::query::constant::int64_to_double(r_test3) << std::endl;


    std::string date1 = "ZONED_DATETIME('-2024-10-01T12:30:20Z')";
    std::string date2 = "2024-01-01T00:00:00Z";
    int64_t r_date1, r_date2;
    std::cout << ring::query::constant::is_date(date1, r_date1) << std::endl;
    std::cout << r_date1 << std::endl;
    std::cout << ring::query::constant::is_date(date2, r_date2) << std::endl;
    std::cout << r_date2 << std::endl;

    std::cout << "Date1: " << ring::query::constant::int64_to_date(r_date1) << std::endl;
    std::cout << "Date2: " << ring::query::constant::int64_to_date(r_date2) << std::endl;

    int64_t proba = 4632323920094800248;
    double proba_d = ring::query::constant::int64_to_double(proba);
    std::cout << "Proba: " << proba << " -> " << proba_d << std::endl;

    proba_d = 120.656075;
    proba = ring::query::constant::double_to_int64(proba_d);
    std::cout << "Proba: " << proba_d << " -> " << proba << std::endl;
     std::cout << "Proba: " << proba << " -> " << ring::query::constant::int64_to_double(proba) << std::endl;

}
