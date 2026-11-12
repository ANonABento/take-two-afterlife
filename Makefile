.PHONY: test clean

test:
	cmake -S Core -B build
	cmake --build build
	ctest --test-dir build --output-on-failure

clean:
	rm -rf build
