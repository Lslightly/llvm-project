#ifndef NODE_H
#define NODE_H

#include <cstdio>
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

#endif