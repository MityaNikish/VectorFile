#include "pch.h"
//#include "VectorFile.hpp"
//#include <exception>
//
//
//
//template <typename T>
//concept Suitable = std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T> && std::is_move_constructible_v<T>;
//
//template <Suitable T>
//class DifficultType
//{
//	std::unique_ptr<T[]> ptr_;
//	size_t length_;
//
//public:
//	DifficultType() : ptr_(nullptr), length_(0) { }
//	DifficultType(size_t size) : ptr_(new T[size]), length_(size) { }
//	~DifficultType() = default;
//
//	DifficultType(const DifficultType& other)
//	{
//		*this = other;
//	}
//
//	DifficultType& operator=(const DifficultType& other)
//	{
//		length_ = other.length_;
//		std::unique_ptr<T[]> new_ptr(new T[length_]);
//
//		std::swap(ptr_, new_ptr);
//		for (int i = 0; i < length_; i++)
//		{
//			ptr_[i] = other.ptr_[i];
//		}
//		return *this;
//	}
//
//	DifficultType(DifficultType&&) noexcept = default;
//	DifficultType& operator=(DifficultType&&) noexcept = default;
//
//
//	T& operator[](size_t index)
//	{
//		if (index < 0 || index >= length_)
//		{
//			throw std::out_of_range("Index goes out of score range.");
//		}
//		return ptr_[index];
//	}
//
//	void push_back(T value)
//	{
//		length_++;
//		std::unique_ptr<T[]> new_ptr(new T[length_]);
//		for (int i = 0; i < length_ - 1; i++)
//		{
//			new_ptr[i] = ptr_[i];
//		}
//		new_ptr[length_ - 1] = value;
//		std::swap(ptr_, new_ptr);
//	}
//
//	T pop_back()
//	{
//		if (length_ == 0)
//		{
//			throw std::runtime_error("Empty type.");
//		}
//		T value = ptr_[length_ - 1];
//		length_--;
//		std::unique_ptr<T[]> new_ptr(new T[length_]);
//		for (int i = 0; i < length_; i++)
//		{
//			new_ptr[i] = ptr_[i];
//		}
//		std::swap(ptr_, new_ptr);
//		return value;
//	}
//
//	size_t size() const
//	{
//		return length_;
//	}
//
//	void resize(size_t size)
//	{
//		std::unique_ptr<T[]> new_ptr(new T[size]);
//		const size_t size_for_writing = size > length_ ? length_ : size;
//		for (int i = 0; i < size_for_writing; i++)
//		{
//			new_ptr[i] = ptr_[i];
//		}
//		std::swap(ptr_, new_ptr);
//		length_ = size;
//	}
//
//	template <typename Iter>
//	class IteratorDifficultType
//	{
//		friend class DifficultType;
//		Iter* iter_;
//		size_t pos_;
//
//		IteratorDifficultType(Iter* ptr) : iter_(ptr), pos_(0) { }
//		~IteratorDifficultType() = default;
//
//	public:
//		using iterator_category = std::input_iterator_tag;
//		using value_type = Iter;
//		using difference_type = ptrdiff_t;
//		using reference = Iter&;
//		using pointer = Iter*;
//
//
//		IteratorDifficultType& operator=(const IteratorDifficultType&) = default;
//
//		IteratorDifficultType& operator++()
//		{
//			iter_++;
//			return *this;
//		}
//		
//		IteratorDifficultType operator++(int)
//		{
//			IteratorDifficultType temp = *this;
//			iter_++;
//			return temp;
//		}
//
//		bool operator==(const IteratorDifficultType&) const = default;
//
//		Iter& operator*()
//		{
//			return *iter_;
//		}
//	};
//
//	typedef IteratorDifficultType<T> iterator;
//	typedef IteratorDifficultType<const T> const_iterator;
//
//	iterator begin() noexcept
//	{
//		return ptr_.get();
//	}
//
//	iterator end() noexcept
//	{
//		return ptr_.get() + length_;
//	}
//
//	const_iterator begin() const noexcept
//	{
//		return ptr_.get();
//	}
//
//	const_iterator end() const noexcept
//	{
//		return ptr_.get() + length_;
//	}
//};
//
//
//template <typename  T>
//struct Serializer<DifficultType<T>>
//{
//	static void serialization(std::fstream& file, DifficultType<T>& elem)
//	{
//		size_t size = elem.size();
//		file.write(reinterpret_cast<char*>(&size), sizeof(size_t));
//		for (T& item : elem) {
//			file.write(reinterpret_cast<char*>(&item), sizeof(T));
//		}
//	}
//
//	static void deserialization(std::fstream& file, DifficultType<T>& elem)
//	{
//		size_t size;
//		file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
//		elem.resize(size);
//		for (T& item : elem) {
//			file.read(reinterpret_cast<char*>(&item), sizeof(T));
//		}
//	}
//};
//
//
//TEST(DifficultType, Test_1)
//{
//	DifficultType<int> Type;
//
//	for (int i = 0; i < 10; i++)
//	{
//		Type.push_back(i);
//	}
//
//	for (int i = 9; i >= 0; i--)
//	{
//		EXPECT_EQ(Type.pop_back(), i);
//	}
//}
//
//TEST(DifficultType, Test_2)
//{
//	DifficultType<int> Type(10);
//
//	for (int i = 0; i < 10; i++)
//	{
//		Type.push_back(5);
//	}
//
//	for (int i = 0; i < 10; i++)
//	{
//		Type[i] = 5;
//	}
//
//	for (int i = 0; i < 20; i++)
//	{
//		EXPECT_EQ(Type.pop_back(), 5);
//	}
//}
//
//TEST(DifficultType, Exceptions)
//{
//	DifficultType<int> Type;
//
//	EXPECT_THROW(Type[20], std::out_of_range);
//	EXPECT_THROW(Type.pop_back(), std::runtime_error);
//}
//
//
//TEST(DifficultType, Iterators)
//{
//	DifficultType<int> Type;
//
//	for (int i = 0; i < 10; i++)
//	{
//		Type.push_back(10);
//	}
//
//	for (auto &item : Type)
//	{
//		EXPECT_EQ(item, 10);
//	}
//}
//
//
//TEST(VectorFile_DifficultType, Write)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<DifficultType<int>> vec(p, static_cast<size_t>(512));
//		EXPECT_EQ(vec.size_file(), 512);
//	}
//	{
//		VectorFile<DifficultType<int>> vec(p, true);
//		EXPECT_EQ(vec.size_file(), 512);
//
//		DifficultType<int> Type;
//
//		for (int i = 0; i < 10; i++)
//		{
//			Type.push_back(i);
//		}
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_NO_THROW(vec[i] = Type);
//		}
//		vec.flush();
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], Type);
//		}
//	}
//	std::filesystem::remove(p);
//}