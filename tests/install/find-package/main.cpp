#include <h5pp/h5pp.h>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    h5pp::File file("h5pp-install-find-package.h5", h5pp::FileAccess::REPLACE);

    std::vector<int> values = {1, 2, 3, 4};
    file.dataset("group/values").write(values);
    file.attribute("group/values", "unit").write(std::string("arb"));

    auto roundtrip = file.dataset("group/values").read<std::vector<int>>();
    auto unit      = file.attribute("group/values", "unit").read<std::string>();

    if(roundtrip != values) throw std::runtime_error("dataset roundtrip failed");
    if(unit != "arb") throw std::runtime_error("attribute roundtrip failed");

    return 0;
}
