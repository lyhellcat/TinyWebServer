#include <iostream>
#include "BlockDeque.hpp"
using namespace std;

int main() {
    BlockDeque<int> bd(3213);
    bd.push_back(2323);
    cout << bd.back() << endl;
    cout << "hello " << endl;
}
