//g++ $ME -o $ME.out && ./$ME.out
//g++ -g $ME -o $ME.out && gdb ./$ME.out

#include <iostream>

namespace __hidden__ {
	struct print {
		bool space;
		print() : space(false) {}
		~print() { std::cout << std::endl; }
		
		template<class T>
		print &operator,(const T &rhs) {
			if(space)
				std::cout << ' ';
			else
				space = true;
			std::cout << rhs;
			return *this;
		}
	};
}

#define print __hidden__::print(),

int main() {
	int a = 1, b = 2;
	
	print "Hello world!";
	print "Sum of", a, "and", b, "is", (a + b);
	
	return 0;
}