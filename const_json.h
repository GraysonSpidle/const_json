/* Author: Grayson Spidle
* If you are reading this, then you are given permission to do whatever you want with this
* 
*/

#ifndef __SPIDLE_CONST_JSON_H__
#define __SPIDLE_CONST_JSON_H__

#pragma warning(push)
#pragma warning(disable: 4623 4626 5027) // tells you that a default constructor, assignment operator, move constructor were implicitly deleted
#pragma warning(disable: 4710, justification: "sanity") // tells you something wasn't inlined
#pragma warning(disable: 4711, justification: "sanity") // tells you something was inlined

#ifndef SPIDLE_CONST_JSON_USING
// A convenience macro to make the schema creation process less of a pain
#define SPIDLE_CONST_JSON_USING using const_json::Bool; \
using const_json::Bool_; \
using const_json::Array; \
using const_json::Array_; \
using const_json::Double; \
using const_json::Double_; \
using const_json::Int; \
using const_json::Int_; \
using const_json::Null; \
using const_json::Null_; \
using const_json::Object; \
using const_json::Object_; \
using const_json::String; \
using const_json::String_; \
using const_json::Optional; \
using const_json::Variant;
#endif // SPIDLE_CONST_JSON_USING

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <map>
#include <vector>
#include <sstream>
#include <variant>
#include <cassert>
#include <iomanip>
#include <istream>
#include <ostream>

namespace const_json {
	// The char dtype the library uses for strings, streams, and error details.
	// Change this if you wish for the library to accomodate wider char types.
	using chartype = char;

	// Identifiers for each token type
	enum class JsonTokens : uint8_t {
		Error = 0x0,
		Bool = 0x1,
		Int = 0x2,
		Double = 0x4,
		String = 0x8,
		Null = 0x10,
		Object = 0x20,
		Array = 0x40,
		Variant = 0x80
	};

	struct Formatting {
		enum class IndentStyle {
			Tabs, Spaces
		} style = IndentStyle::Tabs;

		struct IndentSize {
			int value;
			explicit constexpr IndentSize(decltype(value) num) : value(num) {}
		} size = IndentSize { 0 };

		constexpr Formatting(IndentStyle style, IndentSize size) : style(style), size(size) {}
	};

	constexpr Formatting Minified = { Formatting::IndentStyle::Tabs, Formatting::IndentSize{0} };
	constexpr Formatting PrettyTabs = { Formatting::IndentStyle::Tabs, Formatting::IndentSize{1} };
	constexpr Formatting PrettySpaces = { Formatting::IndentStyle::Spaces, Formatting::IndentSize{4} };

	namespace err {
		struct BadTypeError {
			std::reference_wrapper<const std::type_info> expected;
			std::basic_string<chartype> path;

			BadTypeError(const std::basic_string<chartype>& path, const std::type_info& info) : expected(info), path(path) {}
		};
		// The given JSON is incorrect
		struct MalformedInputError {};
		struct NotAllMembersPresentError {
			std::basic_string<chartype> path;
			std::vector<std::basic_string_view<chartype>> membersAbsent;
			NotAllMembersPresentError(const std::basic_string<chartype>& path) : path(path) {}
		};
	}

	// Anything in this namespace is supposed to be private.
	namespace util {
		template <size_t N> requires (N > 0)
		struct StringLiteral {
			static constexpr size_t size = N;
			chartype str[N];

			template <typename char_type> requires (sizeof(chartype) >= sizeof(char_type))
			consteval StringLiteral(const char_type(&chars)[N]) {
				for (size_t i = 0; i < N; i++) {
					str[i] = chars[i];
				}
			}

			constexpr std::basic_string_view<chartype> toStringView() const {
				// Yes it has to be a string_view; no you cannot plug this into an std::string and expect the string to be there at compile time.
				return std::basic_string_view<chartype>(str, N - 1);
			}

			constexpr bool isEmpty() const {
				// Funnily enough, N can NEVER be 0 (even if I remove the requires at the struct template)
				// With string literals, there is always a null char appended to them so even a blank string will have at least 1 char in it.
				return N == 1;
			}
		};

		template <bool b, bool... Bs>
		static constexpr bool any_of = b or any_of<Bs...>;

		template <bool b>
		static constexpr bool any_of<b> = b;

		template <bool b, bool... Bs>
		static constexpr bool all_of = b and all_of<Bs...>;

		template <bool b>
		static constexpr bool all_of<b> = b;

		template <template <typename...> class Template, typename T>
		struct is_specialization_of : std::false_type {};

		template <template <typename...> class Template, typename... Args>
		struct is_specialization_of<Template, Template<Args...>> : std::true_type {};

		template <template <typename...> class Template, typename T>
		concept specialization_of = is_specialization_of<Template, T>::value;

		template <typename T>
		concept specializes_vector_or_map = specialization_of<std::map, T> or specialization_of<std::vector, T>;

		template <typename T, typename V>
		struct variant_contains_type : public std::false_type {};

		template <typename T, template <typename...> class _Variant, typename... Args> requires specialization_of<std::variant, _Variant<Args...>>
		struct variant_contains_type<T, _Variant<Args...>> {
			static constexpr bool value = util::any_of<std::is_same_v<T, Args>...>;
		};

		template <typename T, typename V>
		constexpr bool variant_contains_type_v = variant_contains_type<T, V>::value;

		template <typename _Variant, typename Ty>
		constexpr bool variant_that_contains_type_v = util::specialization_of<std::variant, _Variant> and variant_contains_type_v<Ty, _Variant>;

		template <typename T>
		concept is_const_json_type = requires(const typename T::rettype& r, typename T::rettype& out, std::basic_istream<chartype>&is, std::basic_ostream<chartype>&os) {
			T::name;
			requires std::same_as<const std::basic_string_view<chartype>, decltype(T::name)>;

			typename T::rettype;
			requires std::default_initializable<typename T::rettype>;

			T::token;
			requires std::same_as<const JsonTokens, decltype(T::token)>;

			T::isOptional;
			requires std::same_as<const bool, decltype(T::isOptional)>;

			T::template read<typename T::rettype>(out, is);

			T::template write<Minified, 0>(r, os);
			T::template write<PrettyTabs, 0>(r, os);
			T::template write<PrettySpaces, 0>(r, os);
		};

		template <typename T>
		constexpr bool is_array_or_object = false;

		template <typename T> requires util::is_const_json_type<T>
		constexpr bool is_array_or_object<T> = T::token == JsonTokens::Array or T::token == JsonTokens::Object;

		template <template <class, class> class Comparator, class... Ts>
		struct as_predicate {
			template <class T2>
			using value = std::bool_constant<any_of<Comparator<Ts, T2>::value...>>;
		};

		template <class... Ts>
		struct tppack {
			static constexpr size_t size = sizeof...(Ts);

			template <template <class...> class Receiver>
			using pipe_to = Receiver<Ts...>;

			template <class Pack>
			struct chain_pack {
				template <class... Ts2>
				using _dummy = tppack<Ts..., Ts2...>;

				using value = Pack::template pipe_to<_dummy>;
			};

			template <size_t i>
			struct at {
				template <class T, class... Ts2>
				struct _impl {
					using value = typename _impl<Ts2...>::value;
				};

				template <class T>
				struct _impl<T> { using value = T; };

				template <class T, class... Ts2> requires (sizeof...(Ts) - 1 - sizeof...(Ts2) == i)
				struct _impl<T, Ts2...> { using value = T; };

				using value = typename _impl<Ts...>::value;
			};

			template <template <class> class Predicate>
			struct filter {
				template <class T, class... Ts2>
				struct _first {
					using value = T;
					using the_rest = tppack<Ts2...>;
				};

				template <class...>
				struct _impl;

				template <class... Ts2> requires (1 < sizeof...(Ts2)) and (Predicate<typename _first<Ts2...>::value>::value)
				struct _impl<Ts2...> {
					using value = typename _first<Ts2...>::the_rest::template pipe_to<_impl>::value;
				};

				template <class... Ts2> requires (1 < sizeof...(Ts2)) and (!Predicate<typename _first<Ts2...>::value>::value)
				struct _impl<Ts2...> {
					using value = typename tppack<typename _first<Ts2...>::value>::template chain_pack<typename _first<Ts2...>::the_rest::template pipe_to<_impl>::value>::value;
				};

				template <class T> requires (Predicate<T>::value)
				struct _impl<T> {
					using value = tppack<>;
				};

				template <class T> requires (!Predicate<T>::value)
				struct _impl<T> {
					using value = tppack<T>;
				};

				template <>
				struct _impl<> {
					using value = tppack<>;
				};

				using value = typename _impl<Ts...>::value;
			};

			template <template <class, class> class Comparator, class... Elements>
			using filter_elements = filter<typename as_predicate<Comparator, Elements...>::value>;

			// Discludes consecutive duplicates using the supplied comparator; yields a new tppack without duplicates
			template <template <class, class> class Comparator> 
			struct unique {
				template <class>
				struct _impl;

				template <class _> requires (sizeof...(Ts) > 0)
				struct _impl<_> {
					using _element = typename tppack<Ts...>::template at<0>::value;
					using _result = typename tppack<Ts...>::template filter_elements<Comparator, _element>::value;
					using value = typename tppack<_element>::template chain_pack<typename _result::template unique<Comparator>::value>::value;
				};

				template <class _> requires (sizeof...(Ts) == 0)
				struct _impl<_> {
					using value = tppack<>;
				};

				using value = typename _impl<void>::value;
			};

			template <template <class, class> class Comparator, class... Args> 
			struct set_difference {
				static_assert(sizeof...(Ts) > 0);

				template <class Arg>
				struct _filter {
					static constexpr bool value = any_of<Comparator<Ts, Arg>::value...>;
				};

				using value = typename tppack<Args...>::template filter<_filter>::value::template unique<Comparator>::value;
			};

			template <template <class, class> class Comparator, class... Args> 
			struct set_symmetric_difference {
				using _left = typename tppack<Args...>::template set_difference<Comparator, Ts...>::value;
				using _right = typename tppack<Ts...>::template set_difference<Comparator, Args...>::value;

				using value = typename _left::template chain_pack<_right>::value::template unique<Comparator>::value;
			};

			template <template <class, class> class Comparator, class Pack>
			struct set_symmetric_difference_pack {
				template <class... Ts2>
				using _impl = set_symmetric_difference<Comparator, Ts2...>;

				using value = typename Pack::template pipe_to<_impl>::value;
			};
		};

		template <class L, class R>
		struct compare_rettype : std::bool_constant<(L::token == R::token)> {};

		template <class L, class R> requires (is_array_or_object<L> or L::token == JsonTokens::Variant) and (is_array_or_object<R> or R::token == JsonTokens::Variant)
		struct compare_rettype<L, R> {
			template <class... RArgs>
			struct _dummy : std::bool_constant<L::_helper::template set_symmetric_difference_pack<compare_rettype, typename R::_helper>::value::size == 0> {};//std::bool_constant<all_of<L::helper::template contains<compare_rettype, RArgs>::value...>> {};

			template <class... LArgs>
			struct _dummy2 {};

			static constexpr bool value = (L::token == R::token) and (0 == L::_helper::template set_symmetric_difference_pack<compare_rettype, typename R::_helper>::value::size);
		};

		template <typename T> requires util::is_const_json_type<T>
		using get_rettype = typename T::rettype;

		template <class T>
		struct build_rettype {
			template <class... Ts>
			using _unpack = tppack<typename Ts::rettype...>;

			using _unpacked = typename T::template pipe_to<_unpack>;

			using value = std::conditional_t<(_unpacked::size > 1),
				typename _unpacked::template pipe_to<std::variant>,
				typename _unpacked::template at<0>::value>;
			using value_variant = _unpacked::template pipe_to<std::variant>;
		};

		template <typename T>
		constexpr T max(std::initializer_list<T>&& list) {
			T num = *list.begin();
			for (auto it = list.begin(); it != list.end(); ++it) {
				if (num < *it)
					num = *it;
			}
			return num;
		}

		inline void skipValue(std::basic_istream<chartype>& is) {
			chartype c;
			is >> std::ws >> c;
			switch (c) {
				// skip object
				case '{': {
					int depth = 1;
					while (depth and is) {
						is >> std::ws >> c;
						switch (c) {
							case '\\':
								is.ignore(1);
								break;
							case '{':
								depth++;
								break;
							case '}':
								depth--;
								break;
							default:
								continue;
						}
					}
					if (is.fail() or is.eof()) [[unlikely]]
						throw err::MalformedInputError();
					break;
				}
				// skip array
				case '[': {
					int depth = 1;
					while (depth and is) {
						is >> std::ws >> c;
						switch (c) {
							case '\\':
								is.ignore(1);
								break;
							case '[':
								depth++;
								break;
							case ']':
								depth--;
								break;
							default:
								continue;
						}
					}
					if (is.fail() or is.eof()) [[unlikely]]
						throw err::MalformedInputError();
					break;
				}
				// skip string
				case '"': {
					while (is) {
						is >> std::ws >> c;
						if (c == '\\')
							is.ignore(1);
						else if (c == '"')
							break;
					}
					if (is.fail()) [[unlikely]]
						throw err::MalformedInputError();
					break;
				}
				// skip null
				case 'n':
					// ull is 3 chars
					// intentional fallthrough
				// skip bool
				case 't': {
					is.ignore(3); // rue is 3 chars
#ifdef _DEBUG
					std::streampos pos = is.tellg();
					chartype ch;
					is >> std::ws >> ch;
					assert(ch == ',' or ch == '}' or ch == ']'); // was trying to skip a bool or null but it seems to be malformed
					is.seekg(pos);
#endif // _DEBUG
					break;
				}
				// skip bool
				case 'f': {
					is.ignore(4); // alse is 4 chars
#ifdef _DEBUG
					std::streampos pos = is.tellg();
					chartype ch;
					is >> std::ws >> ch;
					assert(ch == ',' or ch == '}' or ch == ']'); // was trying to skip a bool but it seems to be malformed
					is.seekg(pos);
#endif // _DEBUG
					break;
				}
				// skip int or skip double
				default: {
					if (c >= '0' and c <= '9') {
#ifdef _DEBUG
						unsigned int decimalCounter = 0;
#endif // _DEBUG

						while (is) {
							is >> c;
#ifdef _DEBUG
							if (c == '.')
								decimalCounter++;
#endif // _DEBUG
							if ((c < '0' or c > '9') and c != '.')
								break;
						}

#ifdef _DEBUG
						assert(decimalCounter < 2); // malformed floating point number
#endif // _DEBUG
						break;
					}
					else
						throw err::MalformedInputError();
				}
			}
		}

		template <util::StringLiteral name, class Error, JsonTokens parent>
		inline void concatPath(Error& e, [[maybe_unused]] size_t size = 0) {
			std::basic_ostringstream<chartype> oss;
			if constexpr (parent == JsonTokens::Object) {
				oss << name.toStringView() << '.';
			}
			else if constexpr (parent == JsonTokens::Array) {
				oss << '[' << size << "].";
			}
			oss << e.path;
			e.path = oss.str();
		}

		template <typename rettype>
		struct ReadFunction_t {
			using raw_t = void(*)(rettype&, std::basic_istream<chartype>&);
			raw_t func;

			constexpr ReadFunction_t(raw_t func) : func(func) {}

			inline void operator()(rettype& r, std::basic_istream<chartype>& is) const { func(r, is); }
		};

		template <Formatting, int, typename>
		struct build_variant_write_func {/* Left intentionally blank in an attempt to make the error clearer */};

		// std::variant only has 1 alternative
		template <Formatting fmt, int depth, template <util::is_const_json_type...> class Template, util::is_const_json_type Alternative>
		struct build_variant_write_func<fmt, depth, Template<Alternative>> {
			template <typename rettype> requires (!util::specialization_of<std::variant, rettype>)
			static void write(const rettype& toBeWritten, std::basic_ostream<chartype>& os) {
				Alternative::template write<fmt, depth>(toBeWritten, os);
			}

			template <typename rettype> requires util::specialization_of<std::variant, rettype> and (std::variant_size_v<rettype> == 1)
			static void write(const rettype& toBeWritten, std::basic_ostream<chartype>& os) {
				auto v = std::get<std::variant_alternative_t<0, rettype>>(toBeWritten);
				Alternative::template write<fmt, depth>(v, os);
			}
		};

		// std::variant has 2+ alternatives
		template <Formatting fmt, int depth, template <util::is_const_json_type...> class Template, util::is_const_json_type... Alternatives> requires (sizeof...(Alternatives) > 1)
		struct build_variant_write_func<fmt, depth, Template<Alternatives...>> {
			template <class... Ts>
			struct basic_visitor_t : Ts... {
				using Ts::operator()...;
			};
			// I don't think this deduction guide is needed as of C++20, but I'm going to keep this here anyway.
			template <class... Ts>
			basic_visitor_t(Ts...) -> basic_visitor_t<Ts...>;

			template <class rettype> requires util::specialization_of<std::variant, rettype>
			static void write(const rettype& toBeWritten, std::basic_ostream<chartype>& os) {
				basic_visitor_t visitor {
					// Yeah that's right, pack expansion with lambdas baby!
					[&](const util::get_rettype<Alternatives>& r2) -> void { Alternatives::template write<fmt, depth>(r2, os); }...
				};
				std::visit(visitor, toBeWritten);
			}
		};

		template <Formatting fmt>
		constexpr bool is_formatting_enabled = fmt.size.value > 0;

		template <Formatting fmt, int depth>
		inline void indent(std::basic_ostream<chartype>& os) {
			if constexpr (is_formatting_enabled<fmt> and depth > 0) {
				if constexpr (fmt.style == Formatting::IndentStyle::Tabs) {
					os << std::basic_string<chartype>(depth * fmt.size.value, chartype('\t'));
				}
				else if constexpr (fmt.style == Formatting::IndentStyle::Spaces) {
					os << std::basic_string<chartype>(static_cast<size_t>(static_cast<intmax_t>(depth) * fmt.size.value), chartype(' '));
				}
			}
		}
	}

	// Yields the schema of the member located at the end of the path specified by memberNames. The output is in a member type alias called 'value'.
	template <util::is_const_json_type RootSchema, util::StringLiteral... memberNames> requires (RootSchema::token == JsonTokens::Object or RootSchema::token == JsonTokens::Variant) and (0 < sizeof...(memberNames))
	struct get_member_schema {
		template <util::is_const_json_type Schema, util::StringLiteral memberName, util::StringLiteral... theRest>
		struct _impl {
			using NextSchema = Schema::template get<memberName>;
			using value = typename _impl<NextSchema, theRest...>::value;
		};

		template <util::is_const_json_type Schema, util::StringLiteral memberName>
		struct _impl<Schema, memberName> {
			using value = Schema::template get<memberName>;
		};

		using value = typename _impl<RootSchema, memberNames...>::value;
	};

	template <util::is_const_json_type RootSchema, util::StringLiteral... memberNames>
	using get_member_schema_t = typename get_member_schema<RootSchema, memberNames...>::value;

	namespace util {
		/* We're implementing the getMember() function here, but we must do it in an unorthodox way because
		* of how I've chosen to approach the implementation. The easiest (and cleanest imo) implementation
		* is a recursive one. Unfortunately, C++ templated functions are not as flexible as templeted structs.
		* The functionality of making a specialization going from
		* template <RootSchema, memberName, additionalMemberNames...> to template <RootSchema memberName> is
		* not available to templated functions.
		*/
		
		template <is_const_json_type RootSchema, StringLiteral memberName, StringLiteral... additionalMemberNames>
		struct build_member_getter {
			using rettype = typename get_member_schema<RootSchema, memberName, additionalMemberNames...>::value::rettype;

			rettype& operator()(typename RootSchema::rettype& arg) const {
				using Schema = typename get_member_schema<RootSchema, memberName>::value;
				auto getter1 = build_member_getter<RootSchema, memberName>();
				auto getter2 = build_member_getter<Schema, additionalMemberNames...>();
				return getter2(getter1(arg));
			}

			const rettype& operator()(const typename RootSchema::rettype& arg) const {
				using Schema = typename get_member_schema<RootSchema, memberName>::value;
				auto getter1 = build_member_getter<RootSchema, memberName>();
				auto getter2 = build_member_getter<Schema, additionalMemberNames...>();
				return getter2(getter1(arg));
			}
		};

		template <is_const_json_type RootSchema, StringLiteral memberName>
		struct build_member_getter<RootSchema, memberName> {
			using rettype = typename get_member_schema<RootSchema, memberName>::value::rettype;

			rettype& operator()(typename RootSchema::rettype& arg) const {
				if constexpr (RootSchema::token == JsonTokens::Object) {
					using Param_t = typename decltype(std::begin(arg))::value_type;
					auto key = std::basic_string(memberName.toStringView());
					//assert(std::find(std::begin(arg), std::end(arg), key) != std::end(arg));

					if constexpr (specialization_of<std::variant, RootSchema::rettype_value_type>)
						return std::get<rettype>(arg[key]);
					else
						return arg[key];
				}
				else if constexpr (RootSchema::token == JsonTokens::Variant) {
					assert(std::holds_alternative<rettype>(arg));
					return std::get<rettype>(arg);
				}
				else {
					abort(); // TODO
				}
			}

			const rettype& operator()(const typename RootSchema::rettype& arg) const {
				if constexpr (RootSchema::token == JsonTokens::Object) {
					auto key = std::basic_string(memberName.toStringView());
					assert(std::find(std::cbegin(arg), std::cend(arg), key) != std::cend(arg));

					if constexpr (specialization_of<std::variant, RootSchema::rettype_value_type>)
						return std::get<rettype>(arg.at(key));
					else
						return arg.at(key);
				}
				else if constexpr (RootSchema::token == JsonTokens::Variant) {
					assert(std::holds_alternative<rettype>(arg));
					return std::get<rettype>(arg); // TODO possible compiler error here due to const-nesss
				}
				else {
					abort(); // TODO
				}
			}
		};
	}

	// Using RootSchema as the base, this gets the member specified by the memberNames "path"
	template <util::is_const_json_type RootSchema, util::StringLiteral... memberNames>
	typename get_member_schema_t<RootSchema, memberNames...>::rettype& getMember(typename RootSchema::rettype& arg) {
		return util::build_member_getter<RootSchema, memberNames...>()(arg);
	}

	// Using RootSchema as the base, this gets the member specified by the memberNames "path"
	template <util::is_const_json_type RootSchema, util::StringLiteral... memberNames>
	const typename get_member_schema_t<RootSchema, memberNames...>::rettype& getMember(const typename RootSchema::rettype& arg) {
		return util::build_member_getter<RootSchema, memberNames...>()(arg);
	}

//	============================================================================================================

	template <util::StringLiteral _name>
	struct Bool {
		static constexpr auto name = _name.toStringView();
		using rettype = bool;
		static constexpr JsonTokens token = JsonTokens::Bool;
		static constexpr bool isOptional = false;
		
		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();
			rettype b;
			is >> std::ws >> std::boolalpha >> b;
			if (is.fail()) {
				is.clear();
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}
			out = b;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << std::boolalpha << r;
		}
	};
	using Bool_ = Bool<"">;
	static_assert(util::is_const_json_type<Bool_>, "Bool is non-conforming");

	template <util::StringLiteral _name>
	struct Int {
		static constexpr auto name = _name.toStringView();
		using rettype = intmax_t;
		static constexpr JsonTokens token = JsonTokens::Int;
		static constexpr bool isOptional = false;

		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();
			
#ifdef _DEBUG
			is.unget();
			assert(is.get() != '"');
#endif // _DEBUG

			rettype i;
			is >> std::ws >> i;
			if (is.fail() or is.peek() == '.') {
				is.clear();
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}
			out = i;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << r;
		}
	};
	using Int_ = Int<"">;
	static_assert(util::is_const_json_type<Int_>, "Int is non-conforming");

	template <util::StringLiteral _name>
	struct Double {
		static constexpr auto name = _name.toStringView();
		using rettype = double;
		static constexpr JsonTokens token = JsonTokens::Double;
		static constexpr bool isOptional = false;

		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();

#ifdef _DEBUG
			is.unget();
			assert(is.get() != '"');
#endif // _DEBUG

			rettype d;
			is >> d;
			if (is.fail()) {
				is.clear();
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}
			out = d;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << r;
		}
	};
	using Double_ = Double<"">;
	static_assert(util::is_const_json_type<Double_>, "Double is non-conforming");

	template <util::StringLiteral _name>
	struct String {
		static constexpr auto name = _name.toStringView();
		using rettype = std::basic_string<chartype>;
		static constexpr JsonTokens token = JsonTokens::String;
		static constexpr bool isOptional = false;

		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();
			rettype str;
			str.reserve(14); // picked an arbitrary amount

			chartype c;
			is >> std::ws >> c;

			if (c != '"') {
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}

			while (is) {
				is >> c;
				if (c == '\\')
					is.ignore(1);
				else if (c == '"')
					break;
				str += c;
			}
			str.shrink_to_fit();
			out = str;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << std::quoted(r);
		}
	};
	using String_ = String<"">;
	static_assert(util::is_const_json_type<String_>, "String is non-conforming");

	template <util::StringLiteral _name>
	struct Null {
		static constexpr auto name = _name.toStringView();
		using rettype = std::monostate;
		static constexpr JsonTokens token = JsonTokens::Null;
		static constexpr bool isOptional = false;

		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();

			chartype buf[4];
			is.read(buf, 4);

			chartype nul[] = { chartype('n'), chartype('u'), chartype('l'), chartype('l') };

			if (std::memcmp(nul, buf, 4 * sizeof(chartype)) != 0) [[unlikely]] {
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}
			
			out = rettype();
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << "null";
		}
	};
	using Null_ = Null<"">;
	static_assert(util::is_const_json_type<Null_>, "Null is non-conforming");

	template <util::is_const_json_type T>
	struct Optional {
		static constexpr auto name = T::name;
		static constexpr JsonTokens token = T::token;
		using rettype = typename T::rettype;
		static constexpr bool isOptional = true;

		template <class>
		struct _get_helpers {
			using helper = void;
			using deduped = void;
		};

		template <class _> requires (util::is_array_or_object<T> or (T::token == JsonTokens::Variant))
		struct _get_helpers<_> {
			using helper = typename T::_helper;
			using deduped = typename T::_deduped;
		};

		using helper = typename _get_helpers<void>::helper;
		using deduped = typename _get_helpers<void>::deduped;

		template <class>
		struct _get_obj_variant_specifics {
			template <util::StringLiteral memberName>
			using get = void;
		};

		template <class _> requires (T::token == JsonTokens::Object or T::token == JsonTokens::Variant)
		struct _get_obj_variant_specifics<_> {
			template <util::StringLiteral memberName>
			using get = T::template get<memberName>;
		};

		template <util::StringLiteral memberName>
		using get = typename _get_obj_variant_specifics<void>::template get<memberName>;

		template <class>
		struct _get_element_schema { using value = void; };

		template <class _> requires (_::token == JsonTokens::Object or _::token == JsonTokens::Array)
		struct _get_element_schema<_> { using value = typename _::element_schema; };

		using element_schema = typename _get_element_schema<T>::value;

		template <typename Parent>
		static void read(Parent& out, std::basic_istream<chartype>& is) { T::template read<Parent>(out, is); }

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) { T::template write<fmt, depth>(r, os); }
	};
	static_assert(util::is_const_json_type<Optional<String<"test">>>, "Optional is non-conforming");

	template <util::StringLiteral _name, util::is_const_json_type... Ts> requires (0 < sizeof...(Ts))
	struct Variant {
		static constexpr auto name = _name.toStringView();
		static_assert(name.size() > 1, "Variant name cannot be empty, if you are using this in an Array, don't. Just add more types to the Array.");

		using _helper = util::tppack<Ts...>;

		using _deduped = typename _helper::template unique<util::compare_rettype>::value;

		using rettype = typename util::build_rettype<_deduped>::value_variant;
		static constexpr JsonTokens token = JsonTokens::Variant;
		static constexpr bool isOptional = false;

		template <util::StringLiteral memberName>
		static constexpr bool contains_name = util::any_of<(Ts::name == memberName.toStringView())...>;

		template <util::StringLiteral name> requires (contains_name<name>)
		struct _get_impl {
			template <class T, class... Ts2>
			struct _impl { using value = typename _impl<Ts2...>::value; };

			template <class T, class... Ts2> requires (T::name == name.toStringView())
			struct _impl<T, Ts2...> { using value = T; };

			template <class T>
			struct _impl<T> { using value = T; };

			using value = typename _impl<Ts...>::value;
		};

		template <util::StringLiteral memberName>
		using get = typename _get_impl<memberName>::value;

		static inline util::ReadFunction_t<rettype> _readFunctions[sizeof...(Ts)] = { Ts::template read<rettype>... };

		template <typename _Variant> requires std::is_same_v<_Variant, rettype> or util::variant_that_contains_type_v<_Variant, rettype>
		static void read(_Variant& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();
			rettype rt;
			bool result = false;
			for (size_t i = 0; i < sizeof...(Ts); i++) {
				auto func = _readFunctions[i];
				try {
					func(rt, is);
				}
				catch (err::BadTypeError) {
					// try another
					continue;
				}
				catch (err::NotAllMembersPresentError& e) {
					util::concatPath<_name, decltype(e), token>(e);
					throw e;
				}
				result = true;
				break;
			}
			if (!result) {
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}
			out = rt;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			using write_func_struct = util::build_variant_write_func<fmt, depth, _deduped>;
			write_func_struct::template write<rettype>(r, os);
		}
	};
	static_assert(util::is_const_json_type<Variant<"blah", Int_, String_, Double_>>, "Variant is non-conforming");

	template <util::StringLiteral _name, util::is_const_json_type... Ts> requires (0 < sizeof...(Ts)) and util::all_of<(Ts::name.size() != 0)...>
	struct Object {
		static constexpr auto name = _name.toStringView();
		static constexpr JsonTokens token = JsonTokens::Object;
		// This is necessary because the compiler won't accept an Object as a class template parameter due to the StringLiteral
		using _helper = util::tppack<Ts...>;

		using _deduped = typename _helper::template unique<util::compare_rettype>::value;
		using rettype = std::map<std::basic_string<chartype>, typename util::build_rettype<_deduped>::value>;
		using rettype_value_type = typename rettype::value_type::second_type;
		// Schema for the data type stored by the Object
		using element_schema = std::conditional_t<(_deduped::size > 1), Variant<"element", Ts...>, typename _deduped::template at<0>::value>;
		static constexpr bool isOptional = false;

		static inline util::ReadFunction_t<rettype_value_type> _readFunctions[sizeof...(Ts)] = { Ts::template read<rettype_value_type>... };
		static inline std::basic_string_view<chartype> _memberNames[sizeof...(Ts)] = { Ts::name... };		

		static constexpr size_t _biggestMemberNameSize = util::max<size_t>({ Ts::name.size()... });

		template <util::StringLiteral nameToTest>
		static constexpr bool contains_name = util::any_of<(Ts::name == nameToTest.toStringView())...>;

		template <util::StringLiteral name> requires(contains_name<name>)
		using get = Variant<"temp", Ts...>::template get<name>;

		template <util::StringLiteral memberName>
		using get_member_rettype = util::get_rettype<typename get<memberName>::value>;

		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			bool memberChecklist[sizeof...(Ts)] = { Ts::isOptional... }; // hopefully this gets optimized into a bitset

			std::streampos begin = is.tellg();

			chartype c;
			is >> std::ws >> c;

			if (c != '{') [[unlikely]] {
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}

			rettype o;
			while (is) {
				is >> std::ws >> c;
				if (c == '}') [[unlikely]]
					break;
				else if (c != '"') [[unlikely]] {
					throw err::MalformedInputError();
				}

				std::basic_string<chartype> memberName;
				memberName.reserve(_biggestMemberNameSize);

				is >> c;
				while (is and c != '"') {
					if (c == '\\') {
						is >> c;
						memberName += c;
						is >> c;
						continue;
					}
					memberName += c;
					is >> c;
				}
				
				is >> std::ws >> c;
				if (c != ':') [[unlikely]] {
					throw err::MalformedInputError();
				}

				size_t index = 0;

				if (memberName.size() > _biggestMemberNameSize) {
					util::skipValue(is);
					goto ReadObjectLoopEnd;
				}
				memberName.shrink_to_fit();

				for (; index < sizeof...(Ts); index++) {
					if (_memberNames[index] == memberName)
						break;
				}
				if (index == sizeof...(Ts)) {
					util::skipValue(is);
					goto ReadObjectLoopEnd;
				}

				try {
					rettype_value_type v;
					_readFunctions[index](v, is);

					memberChecklist[index] = true;
					o[memberName] = v;
				}
				catch (err::BadTypeError& e) {
					util::concatPath<_name, decltype(e), token>(e);
					throw e;
				}
				catch (err::NotAllMembersPresentError& e) {
					util::concatPath<_name, decltype(e), token>(e);
					throw e;
				}

			ReadObjectLoopEnd:
				is >> std::ws >> c;
				if (c == ',') [[likely]]
					continue;
				else if (c == '}')
					break;
				else [[unlikely]]
					throw err::MalformedInputError();
			}

			decltype(err::NotAllMembersPresentError::membersAbsent) members;
			for (size_t i = 0; i < sizeof...(Ts); i++) {
				if (!memberChecklist[i]) [[unlikely]] {
					members.push_back(_memberNames[i]);
				}
			}
			if (members.size()) [[unlikely]] {
				err::NotAllMembersPresentError e = { std::basic_string(name) };
				e.membersAbsent = members;
				throw e;
			}
			out = o;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << '{';
			if constexpr (util::is_formatting_enabled<fmt>) {
				os << '\n';
			}
			bool first = true;

			for (auto it = r.cbegin(); it != r.cend(); ++it) {
				if (!first) {
					os << ',';
					if constexpr (util::is_formatting_enabled<fmt>) {
						os << '\n';
					}
				}
				else
					first = false;

				util::indent<fmt, depth + 1>(os);

				os << std::quoted(it->first) << ':';
				if constexpr (util::is_formatting_enabled<fmt>) {
					os << ' ';
				}

				using write_func_struct = util::build_variant_write_func<fmt, depth + 1, _deduped>;
				write_func_struct::template write<decltype(it->second)>(it->second, os);
			}
			if constexpr (util::is_formatting_enabled<fmt>) {
				if (r.size() > 0)
					os << '\n';
			}
			util::indent<fmt, depth>(os);
			os << '}';
		}
	};
	template <typename... Ts>
	using Object_ = Object<"", Ts...>;
	static_assert(util::is_const_json_type<Object_<Int<"1">, String<"2">, Double<"3">, Bool<"4">, Int<"5">>>, "Object is non-conforming");

	template <util::StringLiteral _name, util::is_const_json_type... Ts> requires (0 < sizeof...(Ts))
	struct Array {
		static constexpr auto name = _name.toStringView();
		static constexpr JsonTokens token = JsonTokens::Array;
		using _helper = util::tppack<Ts...>;

		using _deduped = typename _helper::template unique<util::compare_rettype>::value;
		using rettype = std::vector<typename util::build_rettype<_deduped>::value>;
		// Schema for the data type stored by the Array
		using element_schema = std::conditional_t<(_deduped::size > 1), Variant<"element", Ts...>, typename _deduped::template at<0>::value>;
		static constexpr bool isOptional = false;

		static inline util::ReadFunction_t<typename rettype::value_type> _readFunctions[sizeof...(Ts)] = { Ts::template read<typename rettype::value_type>... };

		template <typename T> requires std::is_same_v<T, rettype> or std::is_assignable_v<T, rettype>
		static void read(T& out, std::basic_istream<chartype>& is) {
			std::streampos begin = is.tellg();

			chartype c;
			is >> std::ws >> c;
			if (c != '[') [[unlikely]] {
				is.seekg(begin);
				throw err::BadTypeError(std::basic_string(name), typeid(rettype));
			}

			rettype a;
			while (is) {
				is >> std::ws;
				if (is.peek() == ']') [[unlikely]] {
					is.ignore(1);
					break;
				}
				
				bool result = false;
				typename rettype::value_type v;
				for (size_t i = 0; i < sizeof...(Ts); i++) {
					try {
						auto func = _readFunctions[i];
						func(v, is);
						result = true;
						break;
					}
					catch (err::BadTypeError) {
						continue;
					}
					catch (err::NotAllMembersPresentError& e) {
						util::concatPath<_name, decltype(e), token>(e, a.size());
						throw e;
					}
				}
				if (!result) [[unlikely]] {
					throw err::BadTypeError(std::basic_string(name), typeid(typename rettype::value_type));
				}
				a.push_back(v);

				is >> std::ws >> c;
				if (c == ',') [[likely]]
					continue;
				else if (c == ']')
					break;
				else [[unlikely]] {
					throw err::MalformedInputError();
				}
			}
			out = a;
		}

		template <Formatting fmt, int depth>
		static void write(const rettype& r, std::basic_ostream<chartype>& os) {
			os << '[';
			if constexpr (util::is_formatting_enabled<fmt>) {
				os << '\n';
			}
			bool first = true;
			for (auto it = r.cbegin(); it != r.cend(); ++it) {
				if (!first) {
					os << ',';
					if constexpr (util::is_formatting_enabled<fmt>) {
						os << '\n';
					}
				}
				else
					first = false;

				util::indent<fmt, depth + 1>(os);

				using write_func_struct = util::build_variant_write_func<fmt, depth + 1, _deduped>;
				write_func_struct::template write<typename rettype::value_type>(*it, os);
			}
			if constexpr (util::is_formatting_enabled<fmt>) {
				if (r.size() > 0)
					os << '\n';
			}
			util::indent<fmt, depth>(os);
			os << ']';
		}
	};
	template <typename... Ts>
	using Array_ = Array<"", Ts...>;
	static_assert(util::is_const_json_type<Array_<Int_, String_, Double_, Bool_, Int_>>, "Array is non-conforming");

// ===========================================================================================================================================

	// Reads the given istream as JSON
	template <util::is_const_json_type Schema>
	typename Schema::rettype parse(std::basic_istream<chartype>& is) {
		typename Schema::rettype out;
		Schema::template read<decltype(out)>(out, is);
		return out;
	}

	// Writes the given Schema rettype to the given ostream according to the given formatting
	template <util::is_const_json_type Schema, Formatting formatting = Minified>
	void dump(const typename Schema::rettype& toBeWritten, std::basic_ostream<chartype>& os) {
		constexpr int initialDepth = 0;
		Schema::template write<formatting, initialDepth>(toBeWritten, os);
	}

	// Writes the given Schema rettype to a string according to the given formatting
	template <util::is_const_json_type Schema, Formatting formatting = Minified>
	std::basic_string<chartype> dump(const typename Schema::rettype& toBeWritten) {
		std::basic_ostringstream<chartype> oss;
		oss << std::fixed;

		constexpr int initialDepth = 0;
		Schema::template write<formatting, initialDepth>(toBeWritten, oss);

		return oss.str();
	}
}
#pragma warning(pop)
#endif // __SPIDLE_CONST_JSON_H__