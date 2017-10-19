std=c++14
libcpp=libc++
files=main.cpp spclock.cpp date/tz.cpp
outfile=clock

main:
	clang++ -Wall -std=$(std) -stdlib=$(libcpp) $(files) -lcurl -o $(outfile)
