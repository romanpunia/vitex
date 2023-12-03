#ifndef VI_ENGINE_GUI_CONFIG_HPP
#define VI_ENGINE_GUI_CONFIG_HPP
#ifdef VI_RMLUI
#include "../../core/core.h"
#include <utility>
#include <vector>
#include <string>
#include <stack>
#include <list>
#include <functional>
#include <queue>
#include <array>
#include <unordered_map>
#include <memory>
#include <RmlUi/Core/Containers/itlib/flat_map.hpp>
#include <RmlUi/Core/Containers/itlib/flat_set.hpp>
#include <RmlUi/Core/Containers/robin_hood.h>
#define RMLUI_MATRIX4_TYPE ColumnMajorMatrix4f
#define RMLUI_RELEASER_FINAL final

namespace Rml
{
	template <typename T, typename Enable = void>
	struct MixedHasher
	{
		inline size_t operator()(const T& Value) const
		{
			auto Result = Mavi::Core::KeyHasher<T>()(Value);
			return robin_hood::hash_int(static_cast<robin_hood::detail::SizeT>(Result));
		}
	};

	template <typename T>
	using Vector = Mavi::Core::Vector<T>;
	template <typename T, size_t N = 1>
	using Array = std::array<T, N>;
	template <typename T>
	using Stack = std::stack<T>;
	template <typename T>
	using List = Mavi::Core::LinkedList<T>;
	template <typename T>
	using Queue = Mavi::Core::SingleQueue<T>;
	template <typename T1, typename T2>
	using Pair = std::pair<T1, T2>;
	template <typename Key, typename Value>
	using UnorderedMultimap = Mavi::Core::UnorderedMultiMap<Key, Value>;
	template <typename Key, typename Value>
	using UnorderedMap = robin_hood::unordered_flat_map<Key, Value, MixedHasher<Key>, Mavi::Core::EqualTo<Key>>;
	template <typename Key, typename Value>
	using SmallUnorderedMap = itlib::flat_map<Key, Value>;
	template <typename T>
	using UnorderedSet = robin_hood::unordered_flat_set<T, MixedHasher<T>, Mavi::Core::EqualTo<T>>;
	template <typename T>
	using SmallUnorderedSet = itlib::flat_set<T>;
	template <typename T>
	using SmallOrderedSet = itlib::flat_set<T>;
	template<typename Iterator>
	inline std::move_iterator<Iterator> MakeMoveIterator(Iterator It)
	{
		return std::make_move_iterator(It);
	}
	template <typename T>
	using Hash = typename Mavi::Core::KeyHasher<T>;
	template <typename T>
	using Function = std::function<T>;
	using String = Mavi::Core::String;
	using StringList = Vector<String>;
	template <typename T>
	using UniquePtr = std::unique_ptr<T>;
	template <typename T>
	class Releaser;
	template <typename T>
	using UniqueReleaserPtr = std::unique_ptr<T, Releaser<T>>;
	template <typename T>
	using SharedPtr = std::shared_ptr<T>;
	template <typename T>
	using WeakPtr = std::weak_ptr<T>;
	template <typename T, typename... Args>
	inline SharedPtr<T> MakeShared(Args&&... Values)
	{
		return std::make_shared<T, Args...>(std::forward<Args>(Values)...);
	}
	template <typename T, typename... Args>
	inline UniquePtr<T> MakeUnique(Args&&... Values)
	{
		return std::make_unique<T, Args...>(std::forward<Args>(Values)...);
	}
}
#endif
#endif