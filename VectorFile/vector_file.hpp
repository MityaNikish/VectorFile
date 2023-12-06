#pragma once
#include <fstream>
#include <vector>
#include <filesystem>
#include "vector_file_exception.hpp"


template <typename T>
concept Acceptable = std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T> && std::is_move_constructible_v<T> && !std::is_const_v<T>;


template <Acceptable T>
class VectorFile final
{
	const size_t type_size_ = sizeof(T);	//размер типа (байт)
	bool is_write_;							//флаг чтение-запись/чтение
	std::fstream file_;						//файл
	std::filesystem::path path_;			//путь к файлу
	size_t file_size_;						//временный размер файла (байт)
	size_t target_file_size_;				//целевой размер файла (байт)
	size_t target_window_size_;				//ширина окна (байт)
	size_t offset_window_;					//смещение окна от начала (байт)
	std::vector<T> buffer_;					//буфер элиментов окна

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
		const size_t number_elem = file_size / type_size_;
		target_file_size_ = number_elem * type_size_;

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
	VectorFile& operator=(VectorFile&&) = delete;
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
		target_file_size_ = new_file_size / type_size_ * type_size_;

		if (new_file_size >= old_file_size)
		{
			return;
		}
		if (file_size_ > new_file_size)
		{
			file_size_ = new_file_size;
		}
	}

	typedef typename std::vector<T>::iterator iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;

	iterator begin() noexcept
	{
		return iterator(buffer_.begin());
	}

	iterator end() noexcept
	{
		return iterator(buffer_.end());
	}


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
		const size_t number_elem = file_size_ - offset_window_ >= target_window_size_ ? target_window_size_ / type_size_ : (file_size_ - offset_window_) / type_size_;

		file_.clear();
		file_.seekg(offset_window_, std::ios::beg);
		for (size_t i = 0; i < number_elem; i++)
		{
			T obj;
			file_.read(reinterpret_cast<char*>(&obj), type_size_);
			buffer_.push_back(obj);
		}
	}

	void write()
	{
		file_.clear();
		file_.seekp(offset_window_, std::ios::beg);
		for (size_t i = 0; i < buffer_.size(); i++)
		{
			file_.write(reinterpret_cast<char*>(&buffer_[i]), type_size_);
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

		T obj;
		for (size_t i = 0; i < number_elem * type_size_; i++)
		{
			file_.write("\0", 1);
		}
		file_size_ += number_elem * type_size_;
	}
};