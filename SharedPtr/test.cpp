#include "SharedPtr.h"
#include <iostream>
#include <cassert>

struct MyObject {
    int value;
    MyObject(int v) : value(v) { std::cout << "MyObject(" << value << ") constructed." << std::endl; }
    ~MyObject() { std::cout << "MyObject(" << value << ") destructed." << std::endl; }
};

void print_line() {
    std::cout << "----------------------------------------" << std::endl;
}

int main() {
    std::cout << "Starting SharedPtr tests..." << std::endl;
    print_line();

    // Test 1: Basic construction and destruction
    std::cout << "Test 1: Basic construction and destruction" << std::endl;
    {
        SharedPtr<MyObject> p1(new MyObject(10));
        assert(p1.UseCount() == 1);
        assert(p1.Unique());
        assert(p1->value == 10);
        std::cout << "p1 goes out of scope..." << std::endl;
    } // p1 destructor should call MyObject destructor
    print_line();

    // Test 2: Copy construction
    std::cout << "Test 2: Copy construction" << std::endl;
    SharedPtr<MyObject> p2(new MyObject(20));
    assert(p2.UseCount() == 1);
    {
        SharedPtr<MyObject> p3 = p2;
        assert(p2.UseCount() == 2);
        assert(p3.UseCount() == 2);
        assert(!p2.Unique());
        assert(p3->value == 20);
        std::cout << "p3 goes out of scope..." << std::endl;
    } // p3 destructor runs, object should NOT be deleted
    assert(p2.UseCount() == 1);
    std::cout << "p2 goes out of scope..." << std::endl;
    print_line();

    // Test 3: Copy assignment
    std::cout << "Test 3: Copy assignment" << std::endl;
    {
        SharedPtr<MyObject> p4(new MyObject(30));
        SharedPtr<MyObject> p5(new MyObject(40));
        assert(p4.UseCount() == 1);
        assert(p5.UseCount() == 1);

        p4 = p5; // p4's object (30) should be destructed
        assert(p4.UseCount() == 2);
        assert(p5.UseCount() == 2);
        assert(p4->value == 40);
        std::cout << "p4 and p5 go out of scope..." << std::endl;
    } // Both point to object(40), which should be destructed now
    print_line();

    // Test 4: Reset
    std::cout << "Test 4: Reset" << std::endl;
    SharedPtr<MyObject> p6(new MyObject(50));
    assert(p6.UseCount() == 1);
    p6.Reset(); // Object(50) should be destructed
    assert(p6.UseCount() == 0);
    assert(p6.Get() == nullptr);
    std::cout << "p6 was reset." << std::endl;
    print_line();

    // Test 5: Move semantics
    std::cout << "Test 5: Move semantics" << std::endl;
    SharedPtr<MyObject> p7(new MyObject(60));
    assert(p7.UseCount() == 1);
    SharedPtr<MyObject> p8 = std::move(p7);
    assert(p7.Get() == nullptr); // p7 should be null
    assert(p7.UseCount() == 0);
    assert(p8.UseCount() == 1);
    assert(p8->value == 60);
    std::cout << "p8 goes out of scope..." << std::endl;
    print_line();

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
