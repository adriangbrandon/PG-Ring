//
// Created by adrian on 12/11/25.
//

#ifndef PROPERTY_GRID_HPP
#define PROPERTY_GRID_HPP
#include <cstdint>
#include <sdsl/construct.hpp>
#include <sdsl/wm_int.hpp>

namespace ring {

    template<class bit_vector_t = sdsl::bit_vector, class wm_bit_vector_t = sdsl::bit_vector>
    class property_grid {

    public:

        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef sdsl::wm_int<wm_bit_vector_t> wm_type;


    private:

        wm_type m_grid; //the grid with the values in Y and the ids in X

        std::pair<value_type, value_type> next(const value_type c_id, std::vector<sdsl::range_type> &ranges) {
            return  m_grid.select_next_pos_with_value(c_id, ranges);
        }

        void copy(const property_grid& o) {
            m_grid = o.m_grid;
        }


    public:

        property_grid() = default;

        //PRE: sorted by id (first) and value (second)
        property_grid(const std::vector<std::pair<value_type, value_type>> &values, const value_type max_id) {
            sdsl::int_vector<> grid_y(max_id+1);
            for (size_type i = 0; i < values.size(); i++) {
                grid_y[values[i].first] = values[i].second;
            }
            grid_y[0] = 0; //dummy
            for (uint32_t i = 0; i < grid_y.size(); i++) {
                std::cout << grid_y[i] << " ";
            }
            std::cout << std::endl;
            sdsl::util::bit_compress(grid_y);
            sdsl::construct_im(m_grid, grid_y);
        }

        property_grid(const property_grid &o) {
            copy(o);
        }

        property_grid(property_grid &&o) noexcept {
            *this = std::move(o);
        }

        property_grid& operator=(const property_grid &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        property_grid& operator=(property_grid &&o) noexcept {
            if (this != &o) {
                m_grid = std::move(o.m_grid);
            }
            return *this;
        }

        void swap(property_grid &o) noexcept{
            m_grid.swap(o.m_grid);
        }

        std::pair<value_type, value_type> next_ge(const value_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{c_value, (1ULL << m_grid.max_level) -1}};
            return next(c_id, ranges);
        }

        std::pair<value_type, value_type> next_se(const value_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{1, c_value}};
            return next(c_id, ranges);
        }

        std::pair<value_type, value_type> next_eq(const value_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{c_value, c_value}};
            auto ans = m_grid.select_next(c_id, ranges);
            return {ans, c_value};
        }

        std::pair<value_type, value_type> next_not_eq(const value_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{1, c_value-1}, {c_value+1, (1ULL << m_grid.max_level) -1}};
            return next(c_id, ranges);
        }

        value_type operator[](const value_type c_id) {
            return m_grid[c_id];
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_grid.serialize(out, child, "grid");
            return written_bytes;
        }

        void load(std::istream &in) {
            m_grid.load(in);
        }

    };

}

#endif //PROPERTY_GRID_HPP