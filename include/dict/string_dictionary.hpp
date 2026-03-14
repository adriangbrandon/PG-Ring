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
        string_dictionary(std::vector<std::string>&& strings,
                         dict_type type = dict_type::HASHRPF,
                         uint overhead = 20,
                         uint bucketsize = 4)
            : m_dict(nullptr) {
            build(std::move(strings), type, overhead, bucketsize);
        }

        /**
         * @brief Load dictionary from file
         * @param filename Path to dictionary file
         * @param opt Optional parameter for some dictionary types
         * @throws std::runtime_error if file cannot be opened or loaded
         */
        explicit string_dictionary(const std::string& filename, uint opt = 0)
            : m_dict(nullptr) {
            load(filename, opt);
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
         * @param strings Vector of sorted strings to include
         * @param type Dictionary type to build
         * @param overhead Hash table overhead (for hash-based dicts, default 20%)
         * @param bucketsize Bucket size (for front-coding dicts, default 4)
         * @throws std::runtime_error if construction fails
         */
        void build(std::vector<std::string> strings,
                   dict_type type = dict_type::HASHRPF,
                   uint overhead = 20,
                   uint bucketsize = 4) {
            if (strings.empty()) {
                throw std::runtime_error("Cannot build dictionary from empty string vector");
            }

            // Assume is alra
            // Sort strings (required by most dictionary implementations)
            //std::sort(strings.begin(), strings.end());

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

            // Create iterator
            IteratorDictString* it = new IteratorDictStringPlain(text, total_len);

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
         * @brief Load dictionary from file
         * @param filename Path to dictionary file
         * @param opt Optional parameter for some dictionary types
         * @throws std::runtime_error if file cannot be opened or loaded
         */
        void load(const std::string& filename, uint opt = 0) {
            std::ifstream in(filename, std::ios::binary);
            if (!in.good()) {
                throw std::runtime_error("Cannot open dictionary file: " + filename);
            }
            load(in, opt);
            in.close();
        }

        /**
         * @brief Load dictionary from input stream
         * @param in Input stream
         * @param opt Optional parameter for some dictionary types
         * @throws std::runtime_error if loading fails
         */
        void load(std::istream& in, uint opt = 0) {
            StringDictionary* dict = StringDictionary::load(dynamic_cast<std::ifstream&>(in), opt);
            if (!dict) {
                throw std::runtime_error("Failed to load dictionary from stream");
            }

            // Clean up old dictionary if exists
            if (m_dict) {
                delete m_dict;
            }

            m_dict = dict;
        }

        /**
         * @brief Save dictionary to file
         * @param filename Path to output file
         * @throws std::runtime_error if file cannot be opened or save fails
         */
        void save(const std::string& filename) const {
            if (!m_dict) {
                throw std::runtime_error("Cannot save: dictionary is empty");
            }
            std::ofstream out(filename, std::ios::binary);
            if (!out.good()) {
                throw std::runtime_error("Cannot open output file: " + filename);
            }
            m_dict->save(out);
            out.close();
        }

        /**
         * @brief Save dictionary to output stream
         * @param out Output stream
         * @throws std::runtime_error if dictionary is empty
         */
        void save(std::ostream& out) const {
            if (!m_dict) {
                throw std::runtime_error("Cannot save: dictionary is empty");
            }
            m_dict->save(dynamic_cast<std::ofstream&>(out));
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
            if (!m_dict) return 0;
            return m_dict->getSize();
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
        template<typename T>
        friend size_type serialize(const string_dictionary& dict, std::ostream& out,
                                   sdsl::structure_tree_node* v = nullptr, std::string name = "") {
            if (!dict.m_dict) {
                // Write a marker indicating empty dictionary
                size_type written_bytes = 0;
                uint8_t empty_marker = 0;
                out.write(reinterpret_cast<const char*>(&empty_marker), sizeof(empty_marker));
                written_bytes += sizeof(empty_marker);

                if (v != nullptr) {
                    sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(
                        v, name, "string_dictionary");
                    sdsl::structure_tree::add_size(child, written_bytes);
                }

                return written_bytes;
            }

            size_type written_bytes = 0;
            auto start_pos = out.tellp();

            // Write marker indicating non-empty dictionary
            uint8_t empty_marker = 1;
            out.write(reinterpret_cast<const char*>(&empty_marker), sizeof(empty_marker));
            written_bytes += sizeof(empty_marker);

            // Save dictionary using libCSD's save method
            dict.m_dict->save(dynamic_cast<std::ofstream&>(out));

            written_bytes = static_cast<size_type>(out.tellp()) - start_pos;

            if (v != nullptr) {
                sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(
                    v, name, "string_dictionary");
                sdsl::structure_tree::add_size(child, written_bytes);
            }

            return written_bytes;
        }

        /**
         * @brief Load dictionary from input stream (SDSL-style)
         * @param dict Dictionary to load into
         * @param in Input stream
         */
        template<typename T>
        friend void load(string_dictionary& dict, std::istream& in) {
            // Read empty marker
            uint8_t empty_marker = 0;
            in.read(reinterpret_cast<char*>(&empty_marker), sizeof(empty_marker));

            if (empty_marker == 0) {
                // Empty dictionary
                if (dict.m_dict) {
                    delete dict.m_dict;
                    dict.m_dict = nullptr;
                }
                return;
            }

            // Load dictionary using libCSD's load method
            StringDictionary* new_dict = StringDictionary::load(dynamic_cast<std::ifstream&>(in), 0);

            if (!new_dict) {
                throw std::runtime_error("Failed to load dictionary from stream");
            }

            // Clean up old dictionary if exists
            if (dict.m_dict) {
                delete dict.m_dict;
            }

            dict.m_dict = new_dict;
        }

    };

    /**
     * @brief Load dictionary from file (factory function)
     * @param filename Path to dictionary file
     * @param opt Optional parameter for some dictionary types
     * @return Loaded dictionary
     * @throws std::runtime_error if loading fails
     */
    inline string_dictionary load_string_dictionary(const std::string& filename, uint opt = 0) {
        return string_dictionary(filename, opt);
    }

} // namespace ring

#endif // RING_STRING_DICTIONARY_HPP

