#ifndef _DNS_SERVER_HEAD
#define _DNS_SERVER_HEAD

#include <cinttypes>
#include <string>
#include <vector>
#include <memory>

#include "primitives.h"

namespace mcshub {

struct dns_packet {
	struct header_t {
		enum class rcode_t : std::uint8_t {
			no_error = 0,
			format_error = 1,
			server_failure = 2,
			name_error = 3,
			not_implemented = 4,
			refused = 5,
		};
		std::uint16_t id : 16;
		bool is_response : 1;
		std::uint8_t op_code : 4;
		bool is_authoritative : 1;
		bool is_trancated : 1;
		bool is_recursive : 1;
		bool is_recursion_available : 1;
		std::uint8_t z : 3;
		std::uint8_t rcode : 4;
		////
		mutable std::uint16_t q_count : 16;
		mutable std::uint16_t a_count : 16;
		std::uint16_t ns_count : 16;
		std::uint16_t ar_count : 16;
		
		static constexpr size_t size() noexcept {
			return sizeof(header_t);
		}
		int read(const byte_t bytes[], size_t length);
		int write(byte_t bytes[], size_t length) const;
	} header;
	enum class record_t : std::uint16_t {
		A = 1,
		NS = 2,
		AAAA = 28,
		TXT = 16
	};
	enum class class_t : std::uint16_t {
		IN = 1,
	};
	struct question_t {
		std::string name;
		record_t type : 16;
		class_t qclass : 16;
		size_t size() const noexcept;
		int read(const byte_t bytes[], size_t length);
		int write(byte_t bytes[], size_t length) const;
	};
	struct answer_t : public question_t {
		struct rdata_t {
			virtual record_t type() const noexcept = 0;
			virtual size_t size() const noexcept = 0;
			virtual int read(const byte_t bytes[], size_t length) = 0;
			virtual int write(byte_t bytes[], size_t length) const = 0;
			virtual ~rdata_t() {}
		};
		struct a_rdata : public rdata_t {
			std::uint32_t data;
			constexpr a_rdata(std::uint32_t d = 0) : data(d) {}
			virtual record_t type() const noexcept override;
			virtual size_t size() const noexcept override;
			virtual int read(const byte_t bytes[], size_t length) override;
			virtual int write(byte_t bytes[], size_t length) const override;
		};
		struct aaaa_rdata : public rdata_t {
			byte_t data[16];
			virtual record_t type() const noexcept override;
			virtual size_t size() const noexcept override;
			virtual int read(const byte_t bytes[], size_t length) override;
			virtual int write(byte_t bytes[], size_t length) const override;
		};
		struct ns_txt_rdata : public rdata_t {
			std::string data;
			virtual size_t size() const noexcept override;
			virtual int read(const byte_t bytes[], size_t length) override;
			virtual int write(byte_t bytes[], size_t length) const override;
			virtual ~ns_txt_rdata() override {}
		};
		struct ns_rdata : public ns_txt_rdata {
			virtual record_t type() const noexcept override;
		};
		struct txt_rdata : public ns_txt_rdata {
			virtual record_t type() const noexcept override;
		};

		answer_t() {};
		answer_t(const answer_t & other) = delete;
		answer_t(answer_t && other) : question_t(other) {
			rdata = other.rdata;
			other.rdata = nullptr;
		}

		std::uint32_t ttl;
		const rdata_t * rdata = nullptr;
		int qwrite(byte_t bytes[], size_t length) const {
			return question_t::write(bytes, length);
		}
		int qread(const byte_t bytes[], size_t length) {
			return question_t::read(bytes, length);
		}
		size_t qsize() const noexcept {
			return question_t::size();
		}
		size_t size() const noexcept;
		int read(const byte_t bytes[], size_t length);
		int write(byte_t bytes[], size_t length) const;
		~answer_t() {
			delete rdata;
		}
	};

	std::vector<question_t> questions;
	std::vector<answer_t> answers;

	size_t size() const;
	int read(const byte_t bytes[], size_t length);
	int write(byte_t bytes[], size_t length) const;

	void answer_A(const std::string & name, std::uint32_t ip_addr, std::uint32_t ttl);
	static std::shared_ptr<dns_packet::answer_t::rdata_t> parse_dns_record(const std::string & data);
};

}

#endif // _DNS_SERVER_HEAD
