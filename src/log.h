
#ifndef _MCSHUB_LOG_HEAD
#define _MCSHUB_LOG_HEAD

#include <ostream>
#include <string>
#include <exception>
#include <fstream>
#include <mutex>

namespace mcshub {

enum class log_level {
	none, fatal, error, warning, info, verbose, debug
};

const std::string & log_lvl2str(log_level lvl);

class log_base {
private:
	log_level lvl;
	std::mutex mutex;
	void write_private(log_level level, const std::string & message);
	void write_exception(log_level level, const std::exception & exception);
protected:
	log_base(log_level level) noexcept : lvl(level) {}
	virtual void write(const std::string & message) = 0;
public:
	void set_log_level(log_level level) noexcept {
		lvl = level;
	}
	log_level log_lvl() const noexcept {
		return lvl;
	}
	void fatal(const std::string & message) {
		write_private(log_level::fatal, message);
	}
	void error(const std::string & message) {
		write_private(log_level::error, message);
	}
	void warning(const std::string & message) {
		write_private(log_level::warning, message);
	}
	void info(const std::string & message) {
		write_private(log_level::info, message);
	}
	void verbose(const std::string & message) {
		write_private(log_level::verbose, message);
	}
	void debug(const std::string & message) {
		write_private(log_level::debug, message);
	}
	void fatal(const std::exception & exception) {
		write_exception(log_level::fatal, exception);
	}
	void error(const std::exception & exception) {
		write_exception(log_level::error, exception);
	}
	void warning(const std::exception & exception) {
		write_exception(log_level::warning, exception);
	}
	void info(const std::exception & exception) {
		write_exception(log_level::info, exception);
	}
	void verbose(const std::exception & exception) {
		write_exception(log_level::verbose, exception);
	}
	void debug(const std::exception & exception) {
		write_exception(log_level::debug, exception);
	}
	virtual ~log_base() {};
};

class file_log : public log_base {
private:
	std::ofstream output;
public:
	file_log(const std::string & filename, log_level level) : log_base(level), output(filename) {
		if (!output) {
			output.close();
			throw std::runtime_error("cannot write to '" + filename + "'");
		}
	}
	virtual void write(const std::string & message) override;
	virtual ~file_log() noexcept override {
		output.close();
	}
};

class stdout_log : public log_base {
public:
	stdout_log(log_level level) : log_base(level) {}
	virtual void write(const std::string & message) override;
};

class empty_log : public log_base {
public:
	empty_log() : log_base(log_level::none) {}
	virtual void write(const std::string & message) override {}
};

extern log_base * log;

} // mcshub

#define log_something(lvl, subject) \
		do { \
			if (int(::mcshub::log_level::lvl) <= int(::mcshub::log->log_lvl())) \
				::mcshub::log->lvl(subject); \
		} while (0)

#define log_fatal(subject) \
		log_something(fatal, subject)

#define log_error(subject) \
		log_something(error, subject)

#define log_warning(subject) \
		log_something(warning, subject)

#define log_info(subject) \
		log_something(info, subject)

#define log_verbose(subject) \
		log_something(verbose, subject)

#define log_debug(subject) \
		log_something(debug, subject)

#endif // _MCSHUB_LOG_HEAD
