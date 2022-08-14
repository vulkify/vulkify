#pragma once
#include <ktl/hash_table.hpp>
#include <ktl/kformat.hpp>
#include <vulkify/context/builder.hpp>
#include <vulkify/context/context.hpp>
#include <optional>

namespace example {
enum class KeyAction { eNone, ePressed, eHeld, eReleased };

struct KeyState {
	KeyAction action{};
	vf::Mods mods{};
};

class Base {
	std::optional<vf::Context> m_context;

  public:
	virtual ~Base() = default;

	int operator()();
	int operator()(int argc, char** argv);

	struct Env {
		struct {
			std::string exe{};
			std::string data{};

		} paths{};
		std::vector<std::string_view> args{};

		void init();
		std::string data_path(std::string_view uri) const;
		std::vector<std::byte> bytes(std::string_view uri) const;
		std::string string(std::string_view uri) const;
	};

	struct Input {
		ktl::hash_table<vf::Key, KeyState> keyStates{};

		void onKey(vf::KeyEvent const& key);
		bool update(vf::EventQueue queue);

		KeyAction key_action(vf::Key key) const;
		vf::Mods mods(vf::Key key) const;
		bool pressed(vf::Key key) const { return key_action(key) == KeyAction::ePressed; }
		bool held(vf::Key key) const { return key_action(key) == KeyAction::eHeld; }
		bool released(vf::Key key) const { return key_action(key) == KeyAction::eReleased; }
	};

	struct Scene {
		std::vector<ktl::kunique_ptr<vf::Primitive>> primitives{};

		template <std::derived_from<vf::Primitive> T, typename... Args>
		T& add(Args&&... args) {
			return add<T>(ktl::make_unique<T>(std::forward<Args>(args)...));
		}

		template <std::derived_from<vf::Primitive> T>
		T& add(ktl::kunique_ptr<T>&& t) {
			auto ret = t.get();
			primitives.push_back(std::move(t));
			return *ret;
		}

		template <std::derived_from<vf::Primitive> T>
		T& add(T&& t) {
			auto ut = ktl::make_unique<T>(std::move(t));
			auto ret = ut.get();
			primitives.push_back(std::move(ut));
			return *ret;
		}
	};

	struct Logger {
		enum class LogLevel { eError, eWarn, eInfo };

		static std::string logString(LogLevel level, std::string_view msg);
		static void print(LogLevel level, std::string msg);

		template <typename... Args>
		static void print(LogLevel level, std::string_view fmt, Args const&... args) {
			print(level, ktl::kformat(fmt, args...));
		}

		template <typename... Args>
		static void error(std::string_view fmt, Args const&... args) {
			print(LogLevel::eError, fmt, args...);
		}

		template <typename... Args>
		static void warn(std::string_view fmt, Args const&... args) {
			print(LogLevel::eWarn, fmt, args...);
		}

		template <typename... Args>
		static void info(std::string_view fmt, Args const&... args) {
			print(LogLevel::eInfo, fmt, args...);
		}
	};

	vf::Context& context() { return *m_context; }
	vf::Context const& context() const { return *m_context; }
	vf::GfxDevice const& device() const { return m_context->device(); }

  protected:
	Env env{};
	Input input{};
	Scene scene{};
	[[no_unique_address]] Logger log{};
	vf::Rgba clear{vf::black_v};

	virtual std::string title() const { return "Example"; }
	virtual void configure(vf::Builder&) {}
	virtual bool setup() { return true; }
	virtual void tick(vf::Time dt) = 0;
	virtual void render(vf::Frame const& frame) const {
		for (auto const& primitive : scene.primitives) { frame.draw(*primitive); }
	}

  private:
	int run();
};
} // namespace example
