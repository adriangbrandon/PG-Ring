/*
 * Copyright (C) 2026 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
        typedef bit_vector_t bv_type;
        typedef typename bv_type::rank_1_type rank_1_type;
        typedef typename bv_type::select_1_type select_1_type;


    private:

        bv_type m_exists; //bitvector to know which ids have a property
        rank_1_type m_rank_exists;
        select_1_type m_select_exists;
        wm_type m_grid; //the grid with the values in Y and the ids in X
        value_type m_last; //last id with property

        std::pair<value_type, value_type> next(const value_type c_id, std::vector<sdsl::range_type> &ranges) {
            auto grid_x = m_rank_exists(c_id) + 1;
            if (grid_x > m_last) return {0,0};
            auto pos_val =  m_grid.select_next_pos_with_value(grid_x, ranges);
            if (pos_val.first == 0) return {0,0};
            pos_val.first = m_select_exists(pos_val.first);
            return pos_val;
        }

        void copy(const property_grid& o) {
            m_exists = o.m_exists;
            m_rank_exists = o.m_rank_exists;
            m_select_exists = o.m_select_exists;
            m_rank_exists.set_vector(&m_exists);
            m_select_exists.set_vector(&m_exists);
            m_grid = o.m_grid;
            m_last = o.m_last;
        }


    public:

        property_grid() = default;

        //PRE: sorted by id (first) and value (second)
        property_grid(const std::vector<std::pair<value_type, std::string>> &values, const value_type max_id) {
            sdsl::bit_vector bv_aux(max_id+1, 0);
            sdsl::int_vector<> grid_y(values.size()+1);
            for (size_type i = 0; i < values.size(); i++) {
                bv_aux[values[i].first] = 1;
                grid_y[i+1] = std::stoi(values[i].second); //the value as integer
            }
            grid_y[0] = 0; //dummy
            sdsl::util::bit_compress(grid_y);
            sdsl::construct_im(m_grid, grid_y);
            m_exists = bv_type(bv_aux);
            sdsl::util::init_support(m_rank_exists, &m_exists);
            sdsl::util::init_support(m_select_exists, &m_exists);
            m_last = values.size();
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
                m_exists = std::move(o.m_exists);
                m_rank_exists = std::move(o.m_rank_exists);
                m_select_exists = std::move(o.m_select_exists);
                m_rank_exists.set_vector(&m_exists);
                m_select_exists.set_vector(&m_exists);
                m_grid = std::move(o.m_grid);
                m_last = o.m_last;
            }
            return *this;
        }

        void swap(property_grid &o) noexcept{
            m_grid.swap(o.m_grid);
            m_exists.swap(o.m_exists);
            sdsl::util::swap_support(m_rank_exists, o.m_rank_exists, &m_exists, &o.m_exists);
            sdsl::util::swap_support(m_select_exists, o.m_select_exists, &m_exists, &o.m_select_exists);
            std::swap(m_last, o.m_last);
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
            auto grid_x = m_rank_exists(c_id) + 1;
            if (grid_x > m_last) return {0,0};
            auto pos =  m_grid.select_next(grid_x, ranges);
            if (pos == 0) return {0,0};
            pos = m_select_exists(pos);
            return {pos, c_value};
        }

        std::pair<value_type, value_type> next_not_eq(const value_type c_id, const value_type c_value) {
            if (c_value == 0) {
                std::vector<sdsl::range_type> ranges = {{1, (1ULL << m_grid.max_level) -1}};
                return next(c_id, ranges);
            }
            std::vector<sdsl::range_type> ranges = {{1, c_value-1}, {c_value+1, (1ULL << m_grid.max_level) -1}};
            return next(c_id, ranges);
        }

        value_type operator[](const value_type c_id) {
            if (!m_exists[c_id]) return 0;
            return m_grid[m_rank_exists(c_id+1)];
        }

        std::pair<value_type, value_type> next_exists(const value_type c_id) {
            if (c_id >= m_exists.size()) return {0, 0}; //no more ids
            auto id = c_id;
            size_type rank = m_rank_exists(c_id);
            if (!m_exists[c_id]) {
                if (rank == m_last) return {0, 0}; //no more ids with property
                id = m_select_exists(rank + 1);
            }
            return {id, m_grid[rank + 1]};

        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_exists.serialize(out, child, "exists");
            written_bytes += m_rank_exists.serialize(out, child, "rank");
            written_bytes += m_select_exists.serialize(out, child, "select");
            written_bytes += m_grid.serialize(out, child, "grid");
            written_bytes += sdsl::write_member(m_last, out, child, "last");
            return written_bytes;
        }

        void load(std::istream &in) {
            m_exists.load(in);
            m_rank_exists.load(in, &m_exists);
            m_select_exists.load(in, &m_exists);
            m_grid.load(in);
            sdsl::read_member(m_last, in);
        }

    };

}

#endif //PROPERTY_GRID_HPP