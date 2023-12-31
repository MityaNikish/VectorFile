#pragma once
#include <fstream>
#include <vector>
#include <filesystem>
#include <iterator>
#include "vector_file_exception.hpp"


template <typename T>
struct Serializer
{
	static void serialization(std::fstream& file, T& elem)
	{
		file.write(reinterpret_cast<char*>(&elem), sizeof(T));
	}

	static void deserialization(std::fstream& file, T& elem)
	{
		file.read(reinterpret_cast<char*>(&elem), sizeof(T));
	}

	static size_t get_size_element(std::fstream& file)
	{
		return sizeof(T);
	}
};

//template <typename T>
//concept Acceptable = std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T> && std::is_move_constructible_v<T> && !std::is_const_v<T>;

template <typename T>
concept Acceptable = std::is_move_constructible_v<T> && !std::is_const_v<T>;

//template <typename T, typename Args>
//concept AcceptableSerialization = requires()
//{
//	T::serialization(std::declval<Args>() ...);
//	T::deserialization(std::declval<Args>() ...);
//};

template <Acceptable T, class S = Serializer<T>>
class VectorFile final
{
	static const size_t type_size_ = sizeof(T);	//������ ���� (����)
	bool is_write_;							//���� ������-������/������
	std::fstream file_;						//����
	std::filesystem::path path_;			//���� � �����
	size_t file_size_;						//��������� ������ ����� (����)
	size_t target_file_size_;				//������� ������ ����� (����)
	size_t target_window_size_;				//������ ���� (����)
	size_t offset_window_;					//�������� ���� �� ������ (����)
	std::vector<T> buffer_;					//����� ��������� ����

public:
	explicit VectorFile(std::filesystem::path path, bool is_write = false, size_t window_size = 1024)
		: is_write_(is_write), path_(std::move(path)), target_window_size_(window_size), offset_window_(0)
	{
		std::ios_base::openmode flags;
		if (is_write_) {
			flags = std::ios::in | std::ios::out | std::ios::binary;
		}
		else {
			flags = std::ios::in | std::ios::binary;
		}

		file_.open(path_, flags);
		if (!file_.is_open()) {
			throw std::runtime_error("File does not exist or could not be opened for reading.");
		}

		file_size_ = get_size_file();
		target_file_size_ = file_size_;

		read();
	}

	explicit VectorFile(std::filesystem::path path, size_t file_size, size_t window_size = 1024)
		: is_write_(true), path_(std::move(path)), file_size_(0), target_window_size_(window_size), offset_window_(0)
	{
		target_file_size_ = align_filesize_to_typesize(file_size);

		file_.open(path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

		const size_t count_elem_for_filling = target_file_size_ > target_window_size_ ? target_window_size_ / type_size_ : target_file_size_ / type_size_;
		filling(count_elem_for_filling);
		read();
	}

	~VectorFile()
	{
		if (!file_.is_open())
		{
			return;
		}
		if (is_write_)
		{
			write();
		}
		if (target_file_size_ > file_size_)
		{
			filling((target_file_size_ - file_size_) / type_size_);
		}
		if (file_size(path_) > target_file_size_)
		{
			file_.close();
			resize_file(path_, target_file_size_);
		}
	}

	VectorFile(VectorFile&&) noexcept = default;
	VectorFile& operator=(VectorFile&&) = default;
	VectorFile(const VectorFile&) = delete;
	VectorFile& operator=(const VectorFile&) = delete;

	size_t size_buffer() const noexcept
	{
		return buffer_.size();
	}

	size_t size_file() const noexcept
	{
		return target_file_size_;
	}

	void seek_window(size_t amount_elements)
	{
		if (amount_elements * type_size_ >= target_file_size_)
		{
			throw goind_out_of_file();
		}
		if (amount_elements * type_size_ >= file_size_)
		{
			const size_t count_elem_for_filling = amount_elements * type_size_ + target_window_size_ < target_file_size_ ? amount_elements * type_size_ - file_size_ + target_window_size_ : target_file_size_ - file_size_;
			filling(count_elem_for_filling);
		}
		if (is_write_)
		{
			write();
		}
		offset_window_ = static_cast<off_t>(amount_elements * type_size_);
		read();
	}

	T& operator[](size_t index)
	{
		if (index < 0 || target_file_size_ / type_size_ <= index)
		{
			throw goind_out_of_file();
		}
		const size_t index_first_elem = offset_window_ / type_size_;
		const size_t window_space = target_file_size_ - offset_window_ >= target_window_size_ ? target_window_size_ : target_file_size_ - offset_window_;
		if (index_first_elem <= index && index <= index_first_elem + window_space - 1)
		{
			return buffer_[index - index_first_elem];
		}
		seek_window(index);
		return buffer_[0];
	}

	void flush()
	{
		if (!is_write_)
		{
			throw write_error();
		}
		write();
	}

	void push_back(const T& value)
	{
		if (!is_write_)
		{
			throw write_error();
		}
		if (buffer_.size() < target_window_size_ / type_size_)
		{
			buffer_.push_back(value);
		}
		if (target_file_size_ - file_size_ > 0)
		{
			filling((target_file_size_ - file_size_) / type_size_);
		}
		file_.clear();
		file_.seekp(target_file_size_, std::ios::beg);
		file_.write(reinterpret_cast<const char*>(&value), type_size_);
		target_file_size_ += type_size_;
		file_size_ += type_size_;
	}

	T pop_back() {
		T obj;
		if (!is_write_)
		{
			throw write_error();
		}
		if (target_file_size_ < type_size_)
		{
			throw goind_out_of_file();
		}
		if (target_file_size_ - offset_window_ <= target_window_size_ && buffer_.size() <= (target_file_size_ - offset_window_) / type_size_)
		{
			buffer_.pop_back();
		}
		if (target_file_size_ - file_size_ > 0)
		{
			filling((target_file_size_ - file_size_) / type_size_);
		}
		file_.clear();
		file_.seekg(target_file_size_ - type_size_, std::ios::beg);
		file_.read(reinterpret_cast<char*>(&obj), type_size_);
		target_file_size_ -= type_size_;
		file_size_ -= type_size_;

		return obj;
	}

	void resize(size_t new_file_size)
	{
		size_t old_file_size = target_file_size_;
		target_file_size_ = align_filesize_to_typesize(new_file_size);

		if (new_file_size >= old_file_size)
		{
			return;
		}
		if (file_size_ > new_file_size)
		{
			file_size_ = new_file_size;
		}
	}

	class FileIterator
	{
		std::fstream& file_;
		size_t pos_;

		FileIterator(std::fstream& file, size_t pos) : file_(file), pos_(pos) {}

	public:
		friend class VectorFile;

		using iterator_category = std::input_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		bool operator!=(const FileIterator& other) const
		{
			return pos_ != other.pos_;
		}

		FileIterator& operator++()
		{
			pos_ += S::get_size_element(file_);
			return *this;
		}

		T operator*()
		{
			file_.seekg(pos_);
			T value;
			S::deserialization(file_, value);
			return value;
		}
	};

	FileIterator begin()
	{
		return FileIterator(file_, 0);
	}

	FileIterator end()
	{
		return FileIterator(file_, file_size_);
	}

	const FileIterator begin() const
	{
		return FileIterator(file_, 0);
	}

	const FileIterator end() const
	{
		return FileIterator(file_, file_size_);
	}


private:
	void default_serialization(std::fstream file_, T elem)
	{
		file_.write(reinterpret_cast<char*>(&elem), type_size_);
	}

	void read()
	{
		buffer_.clear();
		const size_t number_elem = file_size_ - offset_window_ >= target_window_size_ ? target_window_size_ / type_size_ : (file_size_ - offset_window_) / type_size_;

		file_.clear();
		file_.seekg(offset_window_, std::ios::beg);
		for (size_t i = 0; i < number_elem; i++)
		{
			T obj;
			S::deserialization(file_, obj);
			//file_.read(reinterpret_cast<char*>(&obj), type_size_);
			buffer_.push_back(obj);
		}


	}

	void write()
	{
		file_.clear();
		file_.seekp(offset_window_, std::ios::beg);
		for (size_t i = 0; i < buffer_.size(); i++)
		{
			S::serialization(file_, buffer_[i]);
			//file_.write(reinterpret_cast<char*>(&buffer_[i]), type_size_);
		}
	}

	size_t get_size_file()
	{
		file_.seekg(0, std::ios::end);
		const size_t file_size = file_.tellg();
		file_.seekg(0, std::ios::beg);
		return file_size;
	}

	void filling(const size_t number_elem)
	{
		file_.clear();
		file_.seekp(0, std::ios::end);

		for (size_t i = 0; i < number_elem * type_size_; i++)
		{
			file_.write("\0", 1);
		}
		file_size_ += number_elem * type_size_;
	}

	size_t align_filesize_to_typesize(size_t original_file_size) const
	{
		return original_file_size / type_size_ * type_size_;
	}
};