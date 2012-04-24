#include "history.hpp"

int main()
{
	history<char> h(50, 50, 0);
	h.publish(h.get_ver(), 2, 0);
	h.publish(h.get_ver(), 3, 0);
	h.publish(h.get_ver(), 4, 0);
	h.publish(h.get_ver(), 5, 0);

}

