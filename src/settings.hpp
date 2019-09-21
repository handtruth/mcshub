#ifndef _SETTINGS_HEAD
#define _SETTINGS_HEAD

#include <cinttypes>
#include <unordered_map>
#include <exception>
#include <memory>
#include <vector>
#include <optional>
#include <istream>

#include <ekutils/mutex_atomic.hpp>
#include <ekutils/property.hpp>
#include <ekutils/epoll_d.hpp>
#include <ekutils/log.hpp>

namespace mcshub {

struct settings {
	std::string address;
	std::uint16_t port;
	unsigned int threads;
	int max_packet_size;
	unsigned long timeout;

	std::string log;
	ekutils::log_level verb;

	bool distributed;

	std::string domain;

	struct basic_record {
		std::string address;
		std::uint16_t port;

		std::string status;
		std::string login;

		bool drop;
		bool mcsman;

		std::unordered_map<std::string, std::string> vars;
	};

	struct server_record : public basic_record {
		std::optional<basic_record> fml;

	private:
		void copy_fml(const std::optional<basic_record> & other) {
			if (other)
				fml = *other;
			else
				fml.reset();
		}
		void move_fml(std::optional<basic_record> && other) {
			if (other)
				fml = std::move(*other);
			else
				fml.reset();
		}
	public:
		server_record() = default;
		server_record(const basic_record & other) : basic_record(other) {}
		server_record(basic_record && other) : basic_record(std::move(other)) {}
		server_record(const std::string & address, std::uint16_t port, const std::string & status,
			const std::string & login, bool drop, bool mcsman,
			const std::unordered_map<std::string, std::string> & vars) :
				basic_record { address, port, status, login, drop, mcsman, vars } {}
		server_record(const server_record & other) :
				basic_record(other) {
			copy_fml(other.fml);
		}
		server_record(server_record && other) :
				basic_record(std::move(other)) {
			move_fml(std::move(other.fml));
		}
		server_record & operator=(const server_record & other) {
			((basic_record &)(*this)) = other;
			copy_fml(other.fml);
			return *this;
		}
		server_record & operator=(const basic_record & other) {
			((basic_record &)(*this)) = other;
			fml.reset();
			return *this;
		}
		server_record & operator=(server_record && other) noexcept {
			((basic_record &)(*this)) = std::move(other);
			move_fml(std::move(other.fml));
			return *this;
		}
		server_record & operator=(basic_record && other) noexcept {
			((basic_record &)(*this)) = std::move(other);
			fml.reset();
			return *this;
		}
	} default_server;

	std::unordered_map<std::string, server_record> servers;

	static void initialize();
	static void init_listener(ekutils::epoll_d & poll);
	void load(const std::string & path);
	void load(std::istream & input);
	static void static_install();
};

extern settings default_conf;
extern settings::basic_record default_record;
extern const ekutils::matomic<std::shared_ptr<const settings>> & conf;

class conf_snap {
	std::shared_ptr<const settings> snap = conf;
public:
	operator const std::shared_ptr<const settings>() & noexcept {
		return snap;
	}
	const settings & operator*() const noexcept {
		return *snap;
	}
	const settings * operator->() const noexcept {
		return &*snap;
	}
};

class config_exception : public std::exception {
private:
	std::string cause;
public:
	config_exception(const char * field, const std::string & message) noexcept :
		cause(std::string("field \"") + field + "\" has incorrect value (" + message + ')') {}

	virtual const char * what() const noexcept override {
		return cause.c_str();
	}
};

void reload_configuration();

void check_domain(std::string & name);

} // mcshub

#endif // _SETTINGS_HEAD
