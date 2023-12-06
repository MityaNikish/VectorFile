#pragma once
#include <fstream>
#include <vector>
#include <string>
#include "vector_file_exception.hpp"


template <typename T>
concept Acceptable = std::is_trivially_copyable_v<T>;


template <Acceptable T>
class VectorFile final
{
	bool is_write_ = true;	//флаг чтение-запись/чтение
	const size_t type_size_ = sizeof(T);	//размер типа (байт)
	std::fstream file_;	//файл
	size_t file_size_;	//размер файла (байт)
	size_t target_window_size_;	//ширина окна (байт)
	off_t offset_;	//смещение от начала (байт)
	std::vector<T> buffer_;	//вектор элиментов входящих в окно

public:
	VectorFile(const std::string& file_name, size_t window_size = 1024)
		: target_window_size_(window_size), offset_(0)
	{
		std::ios_base::openmode flags;
		if (is_write_) {
			flags = std::ios::in | std::ios::out | std::ios::binary;
		}
		else {
			flags = std::ios::in | std::ios::binary;
		}

		file_.open(file_name, flags);
		if (!file_.is_open()) {
			throw std::runtime_error("File does not exist or could not be opened for reading.");
		}

		file_size_ = get_size_file();

		read();
	}

	VectorFile(VectorFile&&) = default;
	VectorFile& operator=(VectorFile&&) = default;
	VectorFile(const VectorFile&) = delete;
	VectorFile& operator=(const VectorFile&) = delete;

	size_t size_buffer() const noexcept
	{
		return buffer_.size();
	}

	size_t size_file() const noexcept
	{
		return file_size_;
	}

	void seek_window(size_t amount_elements)
	{
		if (amount_elements * type_size_ >= file_size_)
		{
			throw goind_out_of_file();
		}
		offset_ = static_cast<off_t>(amount_elements * type_size_);
		read();
	}

	T& operator[](int index)
	{
		const size_t index_fist_elem = offset_ / type_size_;
		const size_t window_space = target_window_size_ / type_size_;
		if (index < 0 || file_size_ / type_size_ < index)
		{
			throw goind_out_of_file();
		}
		if (index_fist_elem <= index && index <= index_fist_elem + window_space - 1)
		{
			return buffer_[index - index_fist_elem];
		}
		seek_window(index);
		return buffer_[0];
	}

	typedef typename std::vector<T>::iterator iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;



	std::istream_iterator<std::string> begin() noexcept
	{
		file_.clear();
		file_.seekg(0, std::ios::beg);
		file_.seekp(0, std::ios::beg);
		return std::istream_iterator<std::string>(file_);
	}

	std::istream_iterator<std::string> end() noexcept
	{
		return std::istream_iterator<std::string>();
	}

	//iterator begin() noexcept
	//{
	//	return iterator(buffer_.begin());
	//}

	//iterator end() noexcept
	//{
	//	return iterator(buffer_.end());
	//}



	const_iterator begin() const noexcept
	{
		return const_iterator(buffer_.begin());
	}

	const_iterator end() const noexcept
	{
		return const_iterator(buffer_.end());
	}

private:
	void read()
	{
		buffer_.clear();
		const size_t number_elem = file_size_ - offset_ >= target_window_size_ ? target_window_size_ / type_size_ : (file_size_ - offset_) / type_size_;

		file_.clear();
		file_.seekg(offset_, std::ios::beg);
		for (size_t i = 0; i < number_elem; i++)
		{
			T obj;
			file_.read(reinterpret_cast<char*>(&obj), type_size_);
			buffer_.push_back(obj);
		}
	}

	size_t get_size_file()
	{
		file_.seekg(0, std::ios::end);
		const size_t file_size = file_.tellg();
		file_.seekg(0, std::ios::beg);
		return file_size;
	}
};