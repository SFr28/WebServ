#include "httpResponse.hpp"

static std::string	generateSessionID()
{
	size_t 			length = 16;
    const char		charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const size_t	max_index = sizeof(charset) - 2;
	std::string		cookie;

    static bool		seeded = false;

    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(NULL)));
        seeded = true;
    }

    for (size_t i = 0; i < length; ++i) {
        cookie += charset[std::rand() % max_index];
    }
	
	return cookie;
}

void	httpResponse::makeCookie()
{
	if (this->_request.getHeader().find("cookie") == this->_request.getHeader().end())
	{
		std::string	sessionID = generateSessionID();
		this->_headerFields["Set-Cookie"] = "sessionID=" + sessionID + "; Path=/; HttpOnly";
		this->_cookies[sessionID] = "Path=/; HttpOnly";
	}
}
