#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include "node.hh"


int main() {
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<uintptr_t> ptrs;

    for (auto i = 0; i < 10; i++) {
        auto ptr = std::make_shared<Node>();
        nodes.push_back(ptr);
        ptrs.push_back(uintptr_t(ptr.get()));
    }

    std::cout << "size: " << sizeof(Node) << std::endl;
    for (auto ptr: ptrs) {
        std::cout << ptr << std::endl;
    }
}