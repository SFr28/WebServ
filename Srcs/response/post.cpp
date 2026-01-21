#include "httpResponse.hpp"

static std::string generateRandomFilename()
{
	size_t 			length = 16;
    const char		charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const size_t	max_index = sizeof(charset) - 2;
	std::string		filename = "";

    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(NULL)));
        seeded = true;
    }

    for (size_t i = 0; i < length; ++i) {
        filename += charset[std::rand() % max_index];
    }

    return filename;
}

static	std::string	generateExtension(std::string &	contentType)
{
		static std::map<std::string, std::string> mimeTypes;

    if (mimeTypes.empty()) {
        mimeTypes["text/html"] = ".html";
        mimeTypes["text/css"] = ".css";
        mimeTypes["application/javascript"] = ".javascript";
        mimeTypes["application/json"] = ".json";
        mimeTypes["image/png"] = ".png";
        mimeTypes["image/jpeg"] = ".jpeg";
        mimeTypes["image/gif"] = ".gif";
        mimeTypes["image/svg+xml"] = ".svg";
        mimeTypes["text/plain"] = ".txt";
        mimeTypes["application/pdf"] = ".pdf";
        mimeTypes["application/zip"] = ".zip";
        mimeTypes["audio/mpeg"] = ".mp3";
        mimeTypes["video/mp4"] = ".mp4";
        mimeTypes["wvideo/webm"] = ".webm";
        mimeTypes["image/x-icon"] = ".ico";
    }

	contentType = toLower_string(contentType);

	std::map<std::string, std::string>::const_iterator	it = mimeTypes.find(contentType);;
	if (it != mimeTypes.end())
		return it->second;

	return "";
}

static std::string	foundDelimiter(std::map<std::string, std::string> & headers)
{
	std::string	contentType = headers["content-type"];
	size_t		pos_delimiter = contentType.find("boundary=");

	if (pos_delimiter == std::string::npos)
		return "";
	
	std::string	delimiter = "--" + contentType.substr(pos_delimiter + 9);

	size_t semicolon = delimiter.find(';');
	if (semicolon != std::string::npos)
		delimiter = delimiter.substr(0, semicolon);

	size_t cr = delimiter.find('\r');
	if (cr != std::string::npos)
		delimiter = delimiter.substr(0, cr);

	return delimiter;
}

static std::vector<size_t>	foundDelimiterPosition(std::string const & delimiter, std::vector<char> const & content)
{
	std::vector<size_t>	pos;
	size_t				tmpPos = 0;

	while (tmpPos + delimiter.size() <= content.size())
	{
		if (std::equal(delimiter.begin(), delimiter.end(), content.begin() + tmpPos))
		{
			pos.push_back(tmpPos);
			tmpPos += delimiter.size();
		}
		else
			tmpPos++;
	}
	
	return pos;
}

static std::string extractFilename(const char* partData, size_t partSize) 
{
	std::string key = "filename=\"";
	const char* start = std::search(partData, partData + partSize, key.begin(), key.end());

	if (start == partData + partSize)
	{
		key = "name=\"";
		start = std::search(partData, partData + partSize, key.begin(), key.end());
		if (start == partData + partSize)
			return "";
	}
		
	start += key.length();
	const char* end = std::find(start, partData + partSize, '\"');
	if (end == partData + partSize)
		return "";

	return std::string(start, end);
}

bool	httpResponse::makePostFile(const char* data, size_t size, const char* headerStart, size_t headerSize)
{
	std::string filename = extractFilename(headerStart, headerSize);

	if (filename.empty())
	{
		std::string extension = generateExtension(this->_postHeaders["content-type"]);
		filename = generateRandomFilename() + extension;
	}

	if (filename.find("..") != std::string::npos)
	{
		this->_statusCode = FORBIDDEN;
		return false;
	}

	if (this->_path.empty() == false && this->_path[this->_path.length() - 1] != '/')
		this->_path += "/";

	filename = this->_path + filename;

	std::ofstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		this->_statusCode = INTERNAL_SERVER_ERROR;
		return false;
	}

	file.write(data, size);
	file.close();

	return true;
}

bool	httpResponse::parseFormData(std::map<std::string, std::string> & headers)
{
	std::string	delimiter = foundDelimiter(headers);
	if (delimiter.empty())
		return false;
	
	const std::vector<char>& body = this->_request.getBody();

	std::vector<size_t>	pos = foundDelimiterPosition(delimiter, body);
	if (pos.size() < 2)
		return false;

	size_t posStart = pos[0] + delimiter.size() + 2; // \r\n
	size_t posEnd = pos[1] - 2; // Remove trailing \r\n

	const char* partStart = &body[posStart];
	size_t partSize = posEnd - posStart;

	const char* end = "\r\n\r\n";
	const char* headersEnd = std::search(partStart, partStart + partSize, "\r\n\r\n", end + 4);
	if (headersEnd == partStart + partSize)
		return false;

	size_t headerLen = headersEnd - partStart;
	const char* fileData = headersEnd + 4;
	size_t fileDataSize = partSize - headerLen - 4;
	if (fileDataSize == 0)
    {
        this->_statusCode = BAD_REQUEST;
        return false;
    }
	return makePostFile(fileData, fileDataSize, partStart, headerLen);
}

bool	httpResponse::parsePost()
{
	std::map<std::string, std::string>	headers = this->_request.getHeader();
	std::string							contentType = headers["content-type"];

	if (contentType.find("multipart/form-data") != std::string::npos)
		return parseFormData(headers);
	if (contentType.find("text/plain") != std::string::npos)
	{
		std::string		filename = this->_path;
		if (filename[filename.size() - 1] != '/')
			filename += "/";
		filename = filename + generateRandomFilename() + ".txt";
		std::ofstream	file(filename.c_str(), std::ios::binary);

		if (file.is_open() == false)
		{
			this->_statusCode = INTERNAL_SERVER_ERROR;
			return false;
		}

		const std::vector<char>& body = this->_request.getBody();
		file.write(&body[0], body.size());
		file.close();

		this->_path = filename;

		return true;
	}
	this->_statusCode = UNSUPPORTED_MEDIA_TYPE;
	return false;
}

//When the method requested is POST
bool	httpResponse::handlePOST()
{
	if (this->_request.getBody().empty() == true)
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}

	if (isDirectory(this->_path) == false)
	{
		this->_statusCode = FORBIDDEN;
		return false;
	}

	if (parsePost() == false)
		return false;

	std::map<std::string, std::string>	header = this->_request.getHeader();
	if (header["expect"] == "100-continue")
	{
		this->_statusCode = CONTINUE;
	}
	else
	{
		this->_statusCode = CREATED;
		std::string	body = "File Upload";
		this->_body.insert(this->_body.end(), body.begin(), body.end());
		this->_headerFields["Content-Type"] = "text/plain";
	}
	return true;
}
