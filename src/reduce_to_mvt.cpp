#include "reduce_to_mvt.hpp"

#include <cstdlib>
#include <stdexcept>
#include <exception>

int main(int argc, char* argv[]) {
    std::string db_name("output.mbtiles");
    if (argc > 1) {
        db_name = std::string(argv[1]);
    } else {
        std::cerr << "Not enough parameters provided." << std::endl;
        return 1;
    }
    mapbox::mrmvt::reduce_to_mvt(db_name);
    return 0;
}
