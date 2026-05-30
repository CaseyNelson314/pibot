#include <unistd.h>
#include <string>
#include <cassert>

inline std::string get_self_url()
{
    char hostname[256];
    const bool success = gethostname(hostname, sizeof hostname) == 0;
    assert(success);
    return std::string(hostname) + ".local";
}
