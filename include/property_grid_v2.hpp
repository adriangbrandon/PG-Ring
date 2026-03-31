//
// Created by adrian on 12/11/25.
//

#ifndef PROPERTY_GRID_V2_HPP
#define PROPERTY_GRID_V2_HPP
#include <cstdint>
#include <sdsl/construct.hpp>
#include <sdsl/wm_int.hpp>

namespace ring {

    template<class bit_vector_t = sdsl::sd_vector<>, class wm_bit_vector_t = sdsl::bit_vector>
    class property_grid_v2 {

    public:

        typedef uint64_t size_type;
        typedef int64_t value_type;
        typedef uint32_t id_type;
        typedef sdsl::wm_int<wm_bit_vector_t> wm_type;
        typedef bit_vector_t bv_type;
        typedef typename bv_type::rank_1_type rank_1_type;
        typedef typename bv_type::select_1_type select_1_type;
        typedef typename bv_type::select_0_type select_0_type;


    private:

        bv_type m_exists; //bitvector to know which ids have a property
        rank_1_type m_rank_exists;
        select_1_type m_select_exists;
        select_0_type m_select_not_exists;
        wm_type m_grid; //the grid with the values in Y and the ids in X
        id_type m_n_points; //n points in the grid
        value_type m_min_val;
        value_type m_max_val;
        size_type m_grid_max_val;


        std::pair<id_type, value_type> next(const id_type c_id, std::vector<sdsl::range_type> &ranges) {
            auto grid_x = m_rank_exists(c_id) + 1;
            if (grid_x > m_n_points) return {0,0};
            auto pos_val =  m_grid.select_next_pos_with_value(grid_x, ranges);
            if (pos_val.first == 0) return {0,0};
            pos_val.first = m_select_exists(pos_val.first);
            pos_val.second += (m_min_val-1); //shift back to original value
            return pos_val;
        }

        void copy(const property_grid_v2& o) {
            m_exists = o.m_exists;
            m_rank_exists = o.m_rank_exists;
            m_select_exists = o.m_select_exists;
            m_select_not_exists = o.m_select_not_exists;
            m_rank_exists.set_vector(&m_exists);
            m_select_exists.set_vector(&m_exists);
            m_select_not_exists.set_vector(&m_exists);
            m_grid = o.m_grid;
            m_n_points = o.m_n_points;
            m_min_val = o.m_min_val;
            m_max_val = o.m_max_val;
        }


    public:

        const value_type &min_val = m_min_val;
        const value_type &max_val = m_max_val;
        const id_type &n_points = m_n_points;

        property_grid_v2() = default;

        //PRE: sorted by id (first) and value (second)
        property_grid_v2(const std::vector<std::pair<id_type, value_type>> &values, const id_type max_id) {
            sdsl::bit_vector bv_aux(max_id+1, 0);
            std::vector<value_type> aux_y(values.size()+1);
            value_type min_val = std::numeric_limits<value_type>::max();
            value_type max_val = std::numeric_limits<value_type>::min();
            for (size_type i = 0; i < values.size(); i++) {
                bv_aux[values[i].first] = 1;
                aux_y[i+1] = values[i].second; //the value as integer
                if (aux_y[i+1] < min_val) min_val = aux_y[i+1];
                if (aux_y[i+1] > max_val) max_val = aux_y[i+1];
            }
            sdsl::int_vector<> grid_y(values.size()+1);
            for (size_type i = 1; i < grid_y.size(); i++) {
                grid_y[i] = aux_y[i] - (min_val-1); //shift to start from 1
                //std::cout << grid_y[i] << std::endl;
            }
            grid_y[0] = 0; //dummy
            m_min_val = min_val;
            m_max_val = max_val;
            sdsl::util::bit_compress(grid_y);
            sdsl::construct_im(m_grid, grid_y);
            m_exists = bv_type(bv_aux);
            sdsl::util::init_support(m_rank_exists, &m_exists);
            sdsl::util::init_support(m_select_exists, &m_exists);
            sdsl::util::init_support(m_select_not_exists, &m_exists);
            m_n_points = values.size();
            m_grid_max_val = (63 >= m_grid.max_level) ? (1ULL << m_grid.max_level) -1 : UINT64_MAX;
        }

        property_grid_v2(const property_grid_v2 &o) {
            copy(o);
        }

        property_grid_v2(property_grid_v2 &&o) noexcept {
            *this = std::move(o);
        }

        property_grid_v2& operator=(const property_grid_v2 &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        property_grid_v2& operator=(property_grid_v2 &&o) noexcept {
            if (this != &o) {
                m_exists = std::move(o.m_exists);
                m_rank_exists = std::move(o.m_rank_exists);
                m_select_exists = std::move(o.m_select_exists);
                m_select_not_exists = std::move(o.m_select_not_exists);
                m_rank_exists.set_vector(&m_exists);
                m_select_exists.set_vector(&m_exists);
                m_select_not_exists.set_vector(&m_exists);
                m_grid = std::move(o.m_grid);
                m_n_points = o.m_n_points;
                m_min_val = o.m_min_val;
                m_max_val = o.m_max_val;
            }
            return *this;
        }

        void swap(property_grid_v2 &o) noexcept{
            m_grid.swap(o.m_grid);
            m_exists.swap(o.m_exists);
            sdsl::util::swap_support(m_rank_exists, o.m_rank_exists, &m_exists, &o.m_exists);
            sdsl::util::swap_support(m_select_exists, o.m_select_exists, &m_exists, &o.m_exists);
            sdsl::util::swap_support(m_select_not_exists, o.m_select_not_exists, &m_exists, &o.m_exists);
            std::swap(m_n_points, o.m_n_points);
            std::swap(m_min_val, o.m_min_val);
            std::swap(m_max_val, o.m_max_val);
        }

        std::pair<id_type, value_type> next_ge(const id_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{static_cast<size_type>(c_value - (m_min_val-1)), m_grid_max_val}};
            return next(c_id, ranges);
        }

        std::pair<id_type, value_type> next_se(const id_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{1, (size_type) (c_value - (m_min_val-1))}};
            return next(c_id, ranges);
        }

        std::pair<id_type, value_type> next_in_range(const id_type c_id, const value_type l, const value_type r) {
            std::vector<sdsl::range_type> ranges = {{(size_type) (l - (m_min_val-1)), (size_type) (r - (m_min_val-1))}};
            return next(c_id, ranges);
        }

        std::pair<id_type, value_type> next_eq(const id_type c_id, const value_type c_value) {
            std::vector<sdsl::range_type> ranges = {{(size_type) (c_value - (m_min_val-1)),
                                                        (size_type) (c_value - (m_min_val-1))}};
            auto grid_x = m_rank_exists(c_id) + 1;
            if (grid_x > m_n_points) return {0,0};
            auto pos =  m_grid.select_next(grid_x, ranges);
            if (pos == 0) return {0,0};
            pos = m_select_exists(pos);
            return {pos, c_value};
        }

        std::pair<id_type, value_type> next_not_eq(const id_type c_id, const value_type c_value) {
            if (c_value == 0) {
                std::vector<sdsl::range_type> ranges = {{1, m_grid_max_val}};
                return next(c_id, ranges);
            }
            size_type aux = c_value - (m_min_val-1);
            std::vector<sdsl::range_type> ranges = {{1, aux-1}, {aux+1, m_grid_max_val}};
            return next(c_id, ranges);
        }

        std::pair<id_type, value_type> next_is_null(const id_type c_id) {
            if (c_id >= m_exists.size()) return {0, 0}; //no more ids
            if (!m_exists[c_id]) return {c_id, 0}; // the value is not needed
            size_type rank = c_id - m_rank_exists(c_id);
            if (rank == m_exists.size() - m_n_points) return {0, 0}; //no more ids with property
            return {m_select_not_exists(rank + 1), 0}; // the value is not needed
        }

        std::pair<id_type, value_type> next_is_not_null(const id_type c_id) {
            if (c_id >= m_exists.size()) return {0, 0}; //no more ids
            if (m_exists[c_id]) return {c_id, 0}; // the value is not needed
            size_type rank = m_rank_exists(c_id);
            if (rank == m_n_points) return {0, 0}; //no more ids with property
            return {m_select_exists(rank + 1), 0}; // the value is not needed
        }

        std::pair<bool, value_type> operator[](const value_type c_id) {
            if (!m_exists[c_id]) return {false, 0};
            return {true, m_grid[m_rank_exists(c_id+1)] + (m_min_val-1)};
        }

        std::pair<id_type, value_type> next_exists(const id_type c_id) {
            if (c_id >= m_exists.size()) return {0, 0}; //no more ids
            auto id = c_id;
            size_type rank = m_rank_exists(c_id);
            if (!m_exists[c_id]) {
                if (rank == m_n_points) return {0, 0}; //no more ids with property
                id = m_select_exists(rank + 1);
            }
            return {id, m_grid[rank + 1] + (m_min_val-1)};
        }

        size_type cnt_values_range(value_type l, value_type r) {
            return m_grid.count_range_search_2d(1, m_n_points, l - (m_min_val-1), r - (m_min_val-1));
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_exists.serialize(out, child, "exists");
            written_bytes += m_rank_exists.serialize(out, child, "rank");
            written_bytes += m_select_exists.serialize(out, child, "select");
            written_bytes += m_select_not_exists.serialize(out, child, "select_not");
            written_bytes += m_grid.serialize(out, child, "grid");
            written_bytes += sdsl::write_member(m_n_points, out, child, "last");
            written_bytes += sdsl::write_member(m_min_val, out, child, "min_val");
            written_bytes += sdsl::write_member(m_max_val, out, child, "max_val");
            return written_bytes;
        }

        void load(std::istream &in) {
            m_exists.load(in);
            m_rank_exists.load(in, &m_exists);
            m_select_exists.load(in, &m_exists);
            m_select_not_exists.load(in, &m_exists);
            m_grid.load(in);
            sdsl::read_member(m_n_points, in);
            sdsl::read_member(m_min_val, in);
            sdsl::read_member(m_max_val, in);
            m_grid_max_val = (63 >= m_grid.max_level) ? (1ULL << m_grid.max_level) -1 : UINT64_MAX;
        }

    };

}

#endif //PROPERTY_GRID_V2_HPP