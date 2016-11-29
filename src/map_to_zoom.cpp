#include "map_to_zoom.hpp"

#include <cstdlib>
#include <stdexcept>

int main(int argc, char* argv[]) {
    std::size_t min_z = 0;
    std::size_t max_z = 16;
    for (std::size_t i = 0; i < argc; ++i) {
        if (std::strcmp(argv[i],"--min") == 0) {
            ++i;
            if (i >= argc) {
                throw std::runtime_error("Not enough arguments provided");
            }
            min_z = static_cast<std::size_t>(std::atoi(argv[i]));
        } else if (std::strcmp(argv[i],"--max") == 0) {
            ++i;
            if (i >= argc) {
                throw std::runtime_error("Not enough arguments provided");
            }
            max_z = static_cast<std::size_t>(std::atoi(argv[i]));
        }
    }
    mapbox::mrmvt::map_to_zoom(min_z, max_z);
    return 0;
}
