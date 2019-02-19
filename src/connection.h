
#ifndef _CONNECTION_HEAD
#define _CONNECTION_HEAD

#include <memory>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace mcshub {

class connection : public std::enable_shared_from_this<connection> {
public:
	typedef std::shared_ptr<connection> pointer;
private:
	asio::ip::tcp::socket sock;
	connection(asio::io_context& cntxt) : sock(cntxt) {}
};

}

#endif // _CONNECTION_HEAD
