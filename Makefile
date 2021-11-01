CXXFLAGS := -ggdb -std=c++20 -Wall

check: test/passed

test/passed: Makefile test/test32_gcc test/test64_gcc test/test32_clang test/test64_clang
	cd test && ./test32_gcc
	cd test && ./test64_gcc
	cd test && ./test32_clang
	cd test && ./test64_clang
	touch test/passed

test/test32_gcc: Makefile test/test.cpp cbor++.h
	g++ -m32 -o $@ $(CXXFLAGS) $(filter %.cpp,$^) -lgtest -lgtest_main

test/test64_gcc: Makefile test/test.cpp cbor++.h
	g++ -m64 -o $@ $(CXXFLAGS) $(filter %.cpp,$^) -lgtest -lgtest_main

test/test32_clang: Makefile test/test.cpp cbor++.h
	clang++ -m32 -o $@ $(CXXFLAGS) $(filter %.cpp,$^) -lgtest -lgtest_main

test/test64_clang: Makefile test/test.cpp cbor++.h
	clang++ -m64 -o $@ $(CXXFLAGS) $(filter %.cpp,$^) -lgtest -lgtest_main

clean:
	rm -f test/test32* test/test64* test/passed
