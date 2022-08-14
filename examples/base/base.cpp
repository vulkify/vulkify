#include <base.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace example {
namespace stdfs = std::filesystem;

namespace {
stdfs::path findData(stdfs::path exe) {
	for (; !exe.empty() && exe.parent_path() != exe; exe = exe.parent_path()) {
		auto ret = exe / "examples/data";
		if (stdfs::is_directory(ret)) { return ret; }
	}
	return {};
}
} // namespace

void Base::Env::init() {
	if (!args.empty()) {
		auto exe = stdfs::path(args[0]);
		paths.exe = exe.generic_string();
		paths.data = findData(stdfs::absolute(exe)).generic_string();
	}
	if (paths.data.empty()) { paths.data = stdfs::current_path().generic_string(); }
}

std::string Base::Env::data_path(std::string_view uri) const { return (stdfs::path(paths.data) / uri).generic_string(); }

std::vector<std::byte> Base::Env::bytes(std::string_view uri) const {
	if (auto file = std::ifstream(data_path(uri), std::ios::ate | std::ios::binary)) {
		auto const size = file.tellg();
		file.seekg({});
		auto ret = std::vector<std::byte>(static_cast<std::size_t>(size));
		file.read(reinterpret_cast<char*>(ret.data()), size);
		return ret;
	}
	return {};
}

std::string Base::Env::string(std::string_view uri) const {
	auto ret = std::string{};
	if (auto file = std::ifstream(data_path(uri))) {
		for (auto line = std::string{}; std::getline(file, line); line.clear()) {
			ret += std::move(line);
			ret += '\n';
		}
	}
	return ret;
}

void Base::Input::onKey(vf::KeyEvent const& key) {
	switch (key.action) {
	case vf::Action::ePress: keyStates[key.key] = {KeyAction::ePressed, key.mods}; break;
	case vf::Action::eRelease: keyStates[key.key] = {KeyAction::eReleased, key.mods}; break;
	default: break;
	}
}

bool Base::Input::update(vf::EventQueue queue) {
	for (auto [_, state] : keyStates) {
		switch (state.action) {
		case KeyAction::ePressed: state = {KeyAction::eHeld}; break;
		case KeyAction::eReleased: state = {}; break;
		default: break;
		}
	}

	for (auto const& event : queue.events) {
		switch (event.type) {
		case vf::EventType::eClose: return false;
		case vf::EventType::eKey: onKey(event.get<vf::EventType::eKey>()); break;
		default: break;
		}
	}

	auto const& w = keyStates[vf::Key::eW];
	if (w.action == KeyAction::eReleased && w.mods == vf::Mod::eCtrl) { return false; }

	return true;
}

KeyAction Base::Input::key_action(vf::Key key) const {
	if (auto it = keyStates.find(key); it != keyStates.end()) { return it->second.action; }
	return KeyAction::eNone;
}

vf::Mods Base::Input::mods(vf::Key key) const {
	if (auto it = keyStates.find(key); it != keyStates.end()) { return it->second.mods; }
	return {};
}

std::string Base::Logger::logString(LogLevel level, std::string_view msg) {
	static constexpr char levels_v[] = {'E', 'W', 'I'};
	auto const levelChar = levels_v[static_cast<int>(level)];
	char timeStr[16]{};
	auto const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&now));
	return ktl::kformat("[{}] {} [{}]", levelChar, msg, timeStr);
}

void Base::Logger::print(LogLevel level, std::string msg) {
	auto const fd = level == LogLevel::eError ? stderr : stdout;
	std::fprintf(fd, "%s\n", logString(level, msg).c_str());
}

int Base::operator()(int argc, char** argv) {
	env.args.reserve(static_cast<std::size_t>(argc));
	for (int i = 0; i < argc; ++i) { env.args.push_back(argv[i]); }
	env.init();
	return run();
}

int Base::operator()() {
	env.paths.data = stdfs::current_path().generic_string();
	return run();
}

int Base::run() {
	auto builder = vf::Builder{};
	configure(builder);
	auto context = builder.set_title(title()).build();
	if (!context) { return EXIT_FAILURE; }
	m_context.emplace(std::move(*context));
	try {
		if (!setup()) { return EXIT_FAILURE; }
		m_context->show();
		while (!m_context->closing()) {
			auto frame = m_context->frame(clear);
			if (!input.update(frame.poll())) { break; }
			tick(frame.dt());
			render(frame);
		}
	} catch (std::exception const& e) {
		log.error("{}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		log.error("Unknown fatal error");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
} // namespace example
