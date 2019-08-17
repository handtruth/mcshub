#ifndef _SETTINGS_HEAD
#define _SETTINGS_HEAD

#include <cinttypes>
#include <unordered_map>
#include <exception>
#include <memory>
#include <vector>
#include <optional>

#include "log.h"
#include "event_pull.h"
#include "dns_server.h"
#include "primitives.h"
#include "mutex_atomic.h"
#include "property.h"

namespace mcshub {

struct config {
	std::string address;
	std::uint16_t port;
	unsigned int threads;
	int max_packet_size;
	unsigned long timeout;

	std::string log;
	log_level verb;

	bool distributed;

	// Скорее всего не будет
	struct dns_module {
		bool use_dns;
		std::uint32_t ttl;
		std::vector<std::shared_ptr<dns_packet::answer_t::rdata_t>> records;
	} dns;

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
	static void init_listener(event & poll);
	void load(const std::string & path);
	static void static_install();
};

extern config default_conf;
extern config::basic_record default_record;
extern const matomic<std::shared_ptr<const config>> & conf;

class conf_snap {
	std::shared_ptr<const config> snap = conf;
public:
	operator const std::shared_ptr<const config>() & noexcept {
		return snap;
	}
	const config & operator*() const noexcept {
		return *snap;
	}
	const config * operator->() const noexcept {
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
