//
// Modern C++ wrapper for libCSD String Dictionaries
//

#ifndef RING_STRING_DICTIONARY_HPP
#define RING_STRING_DICTIONARY_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <sdsl/structure_tree.hpp>
#include <sdsl/io.hpp>
#include <StringDictionary.h>
#include <iterators/IteratorDictStringPlain.h>

#include "file_util.hpp"

namespace ring {

    /**
     * @brief Modern C++ wrapper for libCSD String Dictionaries
     *
     * Provides RAII-compliant interface with modern C++ features:
     * - Automatic memory management
     * - Move semantics
     * - std::string and std::vector interfaces
     * - Type-safe loading from files
     */
    class string_dictionary {
    public:
        using size_type = uint64_t;
        using id_type = size_t;

        // Dictionary types
        enum class dict_type : uint32_t {
            HASHHF      = 11,   // Hash-Huffman dictionaries
            HASHUFFDAC  = 114,  // HashDAC-Huffman dictionary
            HASHRPF     = 12,   // Hash-RePair dictionaries
            HASHRPDAC   = 124,  // HashDAC-RePair dictionary
            PFC         = 211,  // Plain Front-Coding dictionary
            RPFC        = 214,  // Plain Front-Coding dictionary (with RePair)
            HTFC        = 221,  // HuTucker Front-Coding dictionary
            HHTFC       = 222,  // HuTucker Front-Coding dictionary (with Huffman)
            RPHTFC      = 223,  // HuTucker Front-Coding dictionary (with RePair)
            RPDAC       = 3,    // RePair+DAC dictionary
            FMINDEX     = 4,    // FM-Index dictionary
            DXBW        = 5     // XBW dictionary
        };

    private:
        StringDictionary* m_dict;




    public:

        // Constants for compatibility
        static constexpr size_type NORESULT = 0;

        /**
         * @brief Default constructor - creates empty dictionary
         */
        string_dictionary()
            : m_dict(nullptr) {
        }

        /**
         * @brief Constructs from existing StringDictionary pointer (takes ownership)
         */
        explicit string_dictionary(StringDictionary* dict)
            : m_dict(dict) {
        }

        /**
         * @brief Construct dictionary from vector of strings
         * @param strings Vector of strings to include (will be moved from)
         * @param type Dictionary type to build
         * @param overhead Hash table overhead (for hash-based dicts)
         * @param bucketsize Bucket size (for front-coding dicts)
         * @throws std::runtime_error if construction fails
         */
        string_dictionary(std::vector<std::string>& strings,
                         dict_type type = dict_type::HASHRPF,
                         uint overhead = 20,
                         uint bucketsize = 4)
            : m_dict(nullptr) {
            build(strings, type, overhead, bucketsize);
        }

        /**
         * @brief Destructor
         */
        ~string_dictionary() {
            if (m_dict) {
                delete m_dict;
                m_dict = nullptr;
            }
        }

        // Move semantics
        string_dictionary(string_dictionary&& o) noexcept
            : m_dict(o.m_dict) {
            o.m_dict = nullptr;;
        }

        string_dictionary& operator=(string_dictionary&& o) noexcept {
            if (this != &o) {
                // Clean up current resources
                if (m_dict) {
                    delete m_dict;
                }

                // Transfer ownership
                m_dict = o.m_dict;

                // Nullify source
                o.m_dict = nullptr;
            }
            return *this;
        }

        // Copy is disabled (would require serialization/deserialization)
        string_dictionary(const string_dictionary& o) = delete;
        string_dictionary& operator=(const string_dictionary& o) = delete;

        /**
         * @brief Build dictionary from vector of strings
         * @param strings Vector of strings to include
         * @param type Dictionary type to build
         * @param overhead Hash table overhead (for hash-based dicts, default 20%)
         * @param bucketsize Bucket size (for front-coding dicts, default 4)
         * @throws std::runtime_error if construction fails
         */
        void build(std::vector<std::string> &strings,
                   dict_type type = dict_type::HASHRPF,
                   uint overhead = 20,
                   uint bucketsize = 4) {
            if (strings.empty()) {
                throw std::runtime_error("Cannot build dictionary from empty string vector");
            }

            // Sort strings (required by most dictionary implementations)
            std::sort(strings.begin(), strings.end());

            // Convert to null-delimited format
            size_t total_len = 0;
            for (const auto& s : strings) {
                total_len += s.length() + 1; // +1 for '\0'
            }

            // Create plain text representation
            uchar* text = new uchar[total_len];
            size_t pos = 0;
            for (const auto& s : strings) {
                std::memcpy(text + pos, s.c_str(), s.length());
                text[pos + s.length()] = '\0';
                pos += s.length() + 1;
            }
            std::cout << "Total length: " << total_len << std::endl;
            sdsl::util::clear(strings); // Free original vector memory

            // Create iterator
            IteratorDictString* it = new IteratorDictStringPlain(text, total_len);
            std::cout << "Iterator created with " << it->size() << " strings" << std::endl;

            // Build appropriate dictionary type
            StringDictionary* dict = nullptr;

            try {
                switch (type) {
                    case dict_type::HASHHF:
                        dict = new StringDictionaryHASHHF(it, total_len, overhead);
                        break;
                    case dict_type::HASHUFFDAC:
                        dict = new StringDictionaryHASHUFFDAC(it, total_len, overhead);
                        break;
                    case dict_type::HASHRPF:
                        dict = new StringDictionaryHASHRPF(it, total_len, overhead);
                        break;
                    case dict_type::HASHRPDAC:
                        dict = new StringDictionaryHASHRPDAC(it, total_len, overhead);
                        break;
                    case dict_type::PFC:
                        dict = new StringDictionaryPFC(it, bucketsize);
                        break;
                    case dict_type::RPFC:
                        dict = new StringDictionaryRPFC(it, bucketsize);
                        break;
                    case dict_type::HTFC:
                        dict = new StringDictionaryHTFC(it, bucketsize);
                        break;
                    case dict_type::HHTFC:
                        dict = new StringDictionaryHHTFC(it, bucketsize);
                        break;
                    case dict_type::RPHTFC:
                        dict = new StringDictionaryRPHTFC(it, bucketsize);
                        break;
                    case dict_type::RPDAC:
                        dict = new StringDictionaryRPDAC(it);
                        break;
                    case dict_type::FMINDEX:
                        dict = new StringDictionaryFMINDEX(it, false, 32, 64);
                        break;
                    case dict_type::DXBW:
                        dict = new StringDictionaryXBW(it);
                        break;
                    default:
                        delete it;
                        throw std::runtime_error("Unsupported dictionary type");
                }

                if (!dict) {
                    throw std::runtime_error("Failed to build dictionary");
                }

                // Clean up old dictionary if exists
                if (m_dict) {
                    delete m_dict;
                }

                m_dict = dict;

            } catch (...) {
                delete it;
                throw;
            }
        }

        /**
         * @brief Check if dictionary is loaded and valid
         */
        bool is_valid() const noexcept {
            return m_dict != nullptr;
        }

        /**
         * @brief Locate a string and get its ID
         * @param str String to locate
         * @return ID of the string (0 if not found)
         */
        id_type locate(const std::string& str) const {
            if (!m_dict) return NORESULT;
            return m_dict->locate(
                reinterpret_cast<uchar*>(const_cast<char*>(str.c_str())),
                str.length()
            );
        }

        /**
         * @brief Locate a string and get its ID (C-string version)
         * @param str C-string to locate
         * @param len Length of string
         * @return ID of the string (0 if not found)
         */
        id_type locate(const uchar* str, uint len) const {
            if (!m_dict) return NORESULT;
            return m_dict->locate(const_cast<uchar*>(str), len);
        }

        /**
         * @brief Extract string by ID
         * @param id String ID
         * @return String if found, empty string if not found
         */
        std::string extract(id_type id) const {
            if (!m_dict) return "";

            uint str_len = 0;
            uchar* str = m_dict->extract(id, &str_len);

            if (str == nullptr) {
                return "";
            }

            std::string result(reinterpret_cast<char*>(str), str_len);
            delete[] str;
            return result;
        }

        /**
         * @brief Extract string by ID (with success indicator)
         * @param id String ID
         * @param str_out Output parameter for the string
         * @return true if found, false otherwise
         */
        bool extract(id_type id, std::string& str_out) const {
            if (!m_dict) return false;

            uint str_len = 0;
            uchar* str = m_dict->extract(id, &str_len);

            if (str == nullptr) {
                return false;
            }

            str_out.assign(reinterpret_cast<char*>(str), str_len);
            delete[] str;
            return true;
        }

        /**
         * @brief Extract string by ID (raw pointer version)
         * @param id String ID
         * @param str_len Output parameter for string length
         * @return Pointer to string (caller owns memory, must delete[])
         */
        uchar* extract_raw(id_type id, uint* str_len) const {
            if (!m_dict) return nullptr;
            return m_dict->extract(id, str_len);
        }

        /**
         * @brief Locate all IDs with given prefix
         * @param prefix Prefix to search
         * @return Vector of IDs with the prefix
         */
        std::vector<id_type> locate_prefix(const std::string& prefix) const {
            std::vector<id_type> results;
            if (!m_dict) return results;

            IteratorDictID* it = m_dict->locatePrefix(
                reinterpret_cast<uchar*>(const_cast<char*>(prefix.c_str())),
                prefix.length()
            );

            if (it) {
                while (it->hasNext()) {
                    results.push_back(it->next());
                }
                delete it;
            }

            return results;
        }

        /**
         * @brief Locate all IDs containing given substring
         * @param substr Substring to search
         * @return Vector of IDs containing the substring
         */
        std::vector<id_type> locate_substr(const std::string& substr) const {
            std::vector<id_type> results;
            if (!m_dict) return results;

            IteratorDictID* it = m_dict->locateSubstr(
                reinterpret_cast<uchar*>(const_cast<char*>(substr.c_str())),
                substr.length()
            );

            if (it) {
                while (it->hasNext()) {
                    results.push_back(it->next());
                }
                delete it;
            }

            return results;
        }

        /**
         * @brief Get ID by alphabetical rank
         * @param rank Alphabetical rank (1-based)
         * @return ID at given rank
         */
        id_type locate_rank(uint rank) const {
            if (!m_dict) return NORESULT;
            return m_dict->locateRank(rank);
        }

        /**
         * @brief Extract all strings with given prefix
         * @param prefix Prefix to search
         * @return Vector of strings with the prefix
         */
        std::vector<std::string> extract_prefix(const std::string& prefix) const {
            std::vector<std::string> results;
            if (!m_dict) return results;

            IteratorDictString* it = m_dict->extractPrefix(
                reinterpret_cast<uchar*>(const_cast<char*>(prefix.c_str())),
                prefix.length()
            );

            if (it) {
                while (it->hasNext()) {
                    uint str_len = 0;
                    uchar* str = it->next(&str_len);
                    if (str) {
                        results.emplace_back(reinterpret_cast<char*>(str), str_len);
                        delete[] str;
                    }
                }
                delete it;
            }

            return results;
        }

        /**
         * @brief Extract all strings containing given substring
         * @param substr Substring to search
         * @return Vector of strings containing the substring
         */
        std::vector<std::string> extract_substr(const std::string& substr) const {
            std::vector<std::string> results;
            if (!m_dict) return results;

            IteratorDictString* it = m_dict->extractSubstr(
                reinterpret_cast<uchar*>(const_cast<char*>(substr.c_str())),
                substr.length()
            );

            if (it) {
                while (it->hasNext()) {
                    uint str_len = 0;
                    uchar* str = it->next(&str_len);
                    if (str) {
                        results.emplace_back(reinterpret_cast<char*>(str), str_len);
                        delete[] str;
                    }
                }
                delete it;
            }

            return results;
        }

        /**
         * @brief Extract string by alphabetical rank
         * @param rank Alphabetical rank (1-based)
         * @return String if found, empty string if not found
         */
        std::string extract_rank(uint rank) const {
            if (!m_dict) return "";

            uint str_len = 0;
            uchar* str = m_dict->extractRank(rank, &str_len);

            if (str == nullptr) {
                return "";
            }

            std::string result(reinterpret_cast<char*>(str), str_len);
            delete[] str;
            return result;
        }

        /**
         * @brief Extract string by alphabetical rank (with success indicator)
         * @param rank Alphabetical rank (1-based)
         * @param str_out Output parameter for the string
         * @return true if found, false otherwise
         */
        bool extract_rank(uint rank, std::string& str_out) const {
            if (!m_dict) return false;

            uint str_len = 0;
            uchar* str = m_dict->extractRank(rank, &str_len);

            if (str == nullptr) {
                return false;
            }

            str_out.assign(reinterpret_cast<char*>(str), str_len);
            delete[] str;
            return true;
        }

        /**
         * @brief Get all strings in alphabetical order
         * @return Vector of all strings
         */
        std::vector<std::string> extract_all() const {
            std::vector<std::string> results;
            if (!m_dict) return results;

            IteratorDictString* it = m_dict->extractTable();

            if (it) {
                results.reserve(m_dict->numElements());
                while (it->hasNext()) {
                    uint str_len = 0;
                    uchar* str = it->next(&str_len);
                    if (str) {
                        results.emplace_back(reinterpret_cast<char*>(str), str_len);
                        delete[] str;
                    }
                }
                delete it;
            }

            return results;
        }

        /**
         * @brief Get size in bytes
         */
        size_type size_in_bytes() const {
            if (m_dict) return 0;
            return m_dict->getSize();
        }

        /**
         * @brief Get size in elements
         */

        size_t size() const {
            if (m_dict) return 0;
            return m_dict->numElements();
        }

        /**
         * @brief Swap contents with another dictionary
         */
        void swap(string_dictionary& o) noexcept {
            std::swap(m_dict, o.m_dict);
        }

        /**
         * @brief Access to underlying raw pointer (for advanced use)
         * @warning Use with caution - wrapper still owns the pointer
         */
        StringDictionary* raw_ptr() const noexcept {
            return m_dict;
        }

        /**
         * @brief Check if dictionary contains a string
         */
        bool contains(const std::string& str) const {
            return locate(str) != NORESULT;
        }

        /**
         * @brief Bracket operator for convenient lookup
         * @param id String ID
         * @return String if found, empty string if not found
         */
        std::string operator[](id_type id) const {
            return extract(id);
        }

        /**
         * @brief Serialize dictionary to output stream (SDSL-style)
         * @param out Output stream
         * @param v Dictionary to serialize
         * @param name Optional name for structure (unused but kept for SDSL compatibility)
         * @return Number of bytes written
         */
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            if (!m_dict) {
                // Write a marker indicating empty dictionary
                size_type written_bytes = 0;
                uint8_t empty_marker = 1;
                written_bytes += sdsl::write_member(empty_marker, out, v, name + "empty_mark");
                return written_bytes;
            }

            size_type written_bytes = 0;
            uint8_t not_empty_marker = 0;
            written_bytes += sdsl::write_member(not_empty_marker, out, v, name + "empty_mark");

            std::ofstream out_stream("temp.txt", std::ios::out | std::ios::binary);
            m_dict->save(out_stream);
            out_stream.flush(); out_stream.close();
            written_bytes += ::util::file::file_to_ostream("temp.txt", out);
            ::util::file::remove_file("temp.txt");

            if (v != nullptr) {
                sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(
                    v, name, "string_dictionary");
                sdsl::structure_tree::add_size(child, written_bytes);
            }

            return written_bytes;
        }

        /**
         * @brief Load dictionary from input stream (SDSL-style)
         * @param in Input stream
         */
         void load(std::istream& in) {
            // Read empty marker
            uint8_t empty_marker = 0;
            sdsl::read_member(empty_marker, in);

            if (empty_marker == 1) {
                // Empty dictionary
                if (m_dict) {
                    delete m_dict;
                    m_dict = nullptr;
                }
                return;
            }

            StringDictionary* new_dict = nullptr;
            ::util::file::istream_to_file("temp.txt", in);
            std::ifstream temp_in("temp.txt", std::ios::in | std::ios::binary);
            new_dict = StringDictionary::load(temp_in, 0);
            ::util::file::remove_file("temp.txt");

            if (!new_dict) {
                throw std::runtime_error("Failed to load dictionary from stream");
            }

            if (m_dict) {
                delete m_dict;
            }

            m_dict = new_dict;
        }

    };

} // namespace ring

#endif // RING_STRING_DICTIONARY_HPP

