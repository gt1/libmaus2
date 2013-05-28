#include <iostream>
#include <cstdio>

int dlfunc_cpp(int fd, char const * c, size_t const n)
{
	std::string const message(c,c+n);
	std::cerr << "cpp fd is " << fd << " message is " << message << std::endl;
	return fd;
}

extern "C" {
	int dlfunc(int fd, char const * c, size_t const n)
	{
		// fprintf(stderr,"Fd is %d\n",fd);
		dlfunc_cpp(fd,c,n);
		return fd;
	}
}

