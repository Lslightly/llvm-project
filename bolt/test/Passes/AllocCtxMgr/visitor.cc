#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <stdint.h>
class Node {
    int attrs[20];
public:
    Node() {}
    void visit() {
        for (auto attr: attrs) {
            printf("%d ", attr);
        }
    }
};

int main() {
    std::vector<Node*> nodes;
    std::vector<uintptr_t> ptrs;
    for (auto i = 0; i < 10; i++) {
        auto* node = new Node();
        nodes.push_back(node);
        ptrs.push_back(uintptr_t(node));
    }
    std::cout << "size: " << sizeof(Node) << std::endl;
    for (auto ptr: ptrs) {
        std::cout << ptr << std::endl;
    }
    for (auto* node: nodes) {
        delete node;
    }
    return 0;
}