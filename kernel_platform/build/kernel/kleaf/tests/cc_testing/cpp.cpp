
#include <vector>
#include <stdio.h>

class MyClass {
 public:
  MyClass() : myvec_({1, 2, 3}) {}
  void print() const {
    for (int item : myvec_) {
      printf("%d ", item);
    }
    printf("\n");
  }

 private:
  std::vector<int> myvec_;
};

int main() {
  MyClass object;
  object.print();
  return 0;
}
