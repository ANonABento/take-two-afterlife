.PHONY: test clean

test:
	cmake -S Core -B build
	cmake --build build
	ctest --test-dir build --output-on-failure
	python3 -m unittest discover -s Scripts/tests -v

clean:
	rm -rf build
