#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <crc64.hpp>
#include <vector>
#include <fstream>
#include <filesystem>

namespace py = pybind11;
namespace fs = std::filesystem;


constexpr size_t CHUNK_SIZE = 16 * 1024 * 1024;

uint64_t get_file_crc64(const std::string& filepath) {

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: "+ filepath);
    }
    auto simd = crc64::Digest();
    std::vector<char> buffer(CHUNK_SIZE);
    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        simd.update(buffer.data(), file.gcount());
    }
    uint64_t crc = simd.checksum();
    return crc;
}

uint64_t get_string_crc64(const std::string& data) {
    auto simd = crc64::Digest();
    std::vector<char> buffer(data.begin(), data.end());
    simd.update(buffer.data(), buffer.size());
    uint64_t crc = simd.checksum();
    return crc;
}

PYBIND11_MODULE(fastpycrc64, m) {
    m.doc() = "SIMD-Accelerated file crc64 computation python binding";
    m.def("get_string_crc64", &get_string_crc64, "SIMD-accelerated file crc64 computation", py::arg("data"));
    m.def("get_file_crc64", &get_file_crc64, "SIMD-accelerated file crc64 computation", py::arg("filepath"));
}

