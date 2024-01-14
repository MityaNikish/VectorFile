#pragma once
#include <fstream>
#include <vector>
#include <filesystem>
#include <iterator>
#include "vector_file_exception.hpp"


template <typename T>
struct Serializer
{
	static void serialization(std::fstream& file, const T& elem)
	{
		file.write(reinterpret_cast<const char*>(&elem), sizeof(T));
	}

	static void deserialization(std::fstream& file, T& elem)
	{
		file.read(reinterpret_cast<char*>(&elem), sizeof(T));
	}

	static size_t deserialization_size_element(std::fstream& file)
	{
		return sizeof(T);
	}

	static size_t get_size_element(const T& elem)
	{
		return sizeof(T);
	}
};


template <typename T>
concept Acceptable = std::is_move_constructible_v<T> && !std::is_const_v<T>;


template <typename S, typename Arg>
concept AcceptableSerialization = requires(std::fstream& file, Arg& value)
{
	S::serialization(file, value);
	S::deserialization(file, value);
	S::deserialization_size_element(file);
	S::get_size_element(value);
};


template <Acceptable T, class S = Serializer<T>>
requires AcceptableSerialization<S, T>
class VectorFile final
{
	bool is_writable_;							//флаг чтение-запись/чтение
	std::fstream file_;						//файл
	std::filesystem::path path_;			//путь к файлу
	std::streamsize window_size_;				//ширина окна (байт)
	std::streampos pos_window_;					//смещение окна от начала (байт)
	std::streamoff offset_end;          //смещение с конца(байт)
	std::vector<T> buffer_;					//буфер элиментов окна

public:
	explicit VectorFile(std::filesystem::path path, bool is_write = false, size_t window_size = 1024)
		: is_writable_(is_write), path_(path), window_size_(window_size), pos_window_(0), offset_end(0)
	{
		std::ios_base::openmode flags;
		if (is_writable_) {
			flags = std::ios::in | std::ios::out | std::ios::binary;
		}
		else {
			flags = std::ios::in | std::ios::binary;
		}

		file_.open(path_, flags);
		if (!file_.is_open()) {
			throw std::runtime_error("File does not exist or could not be opened for reading.");
		}

		read();
	}

	explicit VectorFile(std::filesystem::path path, size_t file_size, size_t window_size = 1024)
		: is_writable_(true), path_(path), window_size_(window_size), pos_window_(0), offset_end(0)
	{
		file_.open(path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
		resize(file_size);
		read();
	}

	~VectorFile()
	{
		if (!file_.is_open())
		{
			return;
		}
		//if (is_writable_)
		//{
		//	write();
		//}

		std::streamsize file_size = get_size_file();

		file_.close();
		resize_file(path_, file_size);
	}

	VectorFile(VectorFile&&) noexcept = default;
	VectorFile& operator=(VectorFile&&) = default;
	VectorFile(const VectorFile&) = delete;
	VectorFile& operator=(const VectorFile&) = delete;


	size_t file_len() noexcept
	{
		return get_size_file();
	}

	size_t size()
	{
		return get_number_elem(0, get_size_file());
	}

	void seek_window(size_t number_element)
	{
		if (is_writable_)
		{
			write(buffer_, pos_window_);
		}
		pos_window_ = get_pos_elem(0, number_element);
		std::streamsize file_size = get_size_file();
		read();
	}

	T& operator[](size_t index)
	{
		const std::streampos pos_element = get_pos_elem(0, index);
		file_.clear();
		file_.seekp(pos_element, std::ios::beg);
		const std::streampos size_element = S::deserialization_size_element(file_);

		if (pos_element >= pos_window_ && pos_element + size_element < pos_window_ + window_size_)
		{
			return buffer_[get_number_elem(pos_window_, pos_element - pos_window_)];
		}
		seek_window(index);
		return buffer_[0];
	}

	void flush()
	{
		if (!is_writable_)
		{
			throw write_error();
		}
		write(buffer_, pos_window_);
	}

	void push_back(const T& value)
	{
		if (!is_writable_)
		{
			throw write_error();
		}

		const std::streamsize file_size = get_size_file();
		file_.clear();
		file_.seekp(file_size, std::ios::beg);
		const std::streampos pos_new_elem = file_.tellp();
		S::serialization(file_, value);
		const std::streamsize size_new_elem = S::get_size_element(value);

		if (pos_new_elem >= pos_window_ && pos_new_elem + size_new_elem < pos_window_ + window_size_)
		{
			buffer_.push_back(value);
		}
		offset_end = offset_end < size_new_elem ? 0 : offset_end - size_new_elem;
	}

	T pop_back()
	{
		if (!is_writable_)
		{
			throw write_error();
		}

		T value;
		const size_t index_last_elem = size() - 1;
		const std::streampos pos_last_elem =  get_pos_elem(0, index_last_elem);

		file_.clear();
		file_.seekg(pos_last_elem,  std::ios::beg);
		S::deserialization(file_, value);
		const std::streamsize size_elem = S::get_size_element(value);

		if (pos_last_elem >= pos_window_ && pos_last_elem + size_elem < pos_window_ + window_size_)
		{
			buffer_.pop_back();
		}
		offset_end += size_elem;

		return value;
	}

	void resize(const std::streamsize new_file_size)
	{
		const std::streamsize file_size = get_size_file();

		if (new_file_size > file_size)
		{
			filling(new_file_size);
		}
		else if (new_file_size < file_size)
		{
			liberation(new_file_size);
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
			pos_ += S::deserialization_size_element(file_);
			return *this;
		}

		T operator*()
		{
			std::streampos temp = file_.tellg();
			file_.clear();
			file_.seekg(pos_, std::ios::beg);
			T value;
			S::deserialization(file_, value);
			file_.clear();
			file_.seekg(temp, std::ios::beg);
			return value;
		}
	};

	[[nodiscard]] FileIterator begin()
	{
		return FileIterator(file_, 0);
	}

	[[nodiscard]] FileIterator end()
	{
		return FileIterator(file_, get_size_file());
	}

	[[nodiscard]] const FileIterator begin() const
	{
		return FileIterator(file_, 0);
	}

	[[nodiscard]] const FileIterator end() const
	{
		return FileIterator(file_, get_size_file());
	}



private:

	void read(std::vector<T>& buffer, const std::streampos start_pos, const std::streamsize range)
	{
		buffer.clear();
		const std::streamsize file_size = get_size_file();
		std::streampos offset = start_pos;

		while (true)
		{
			if (offset == file_size)
			{
				break;
			}
			file_.clear();
			file_.seekg(offset, std::ios::beg);
			const std::streamsize elem_size = S::deserialization_size_element(file_);
			if (offset + elem_size > start_pos + range)
			{
				break;
			}
			T obj;
			file_.clear();
			file_.seekg(offset, std::ios::beg);
			S::deserialization(file_, obj);
			buffer.push_back(obj);
			offset += elem_size;
		}
	}

	void write(std::vector<T>& buffer, const std::streamsize start_pos)
	{
		const std::streamsize file_size = get_size_file();
		std::streampos offset = start_pos;

		for (size_t i = 0; i < buffer.size(); i++)
		{
			file_.clear();
			file_.seekp(offset, std::ios::beg);
			const std::streamsize elem_size = S::deserialization_size_element(file_);

			if (S::get_size_element(buffer[i]) == elem_size)
			{
				file_.clear();
				file_.seekp(offset, std::ios::beg);
				S::serialization(file_, buffer[i]);
				offset += elem_size;
			}
		}
	}

	void read()
	{
		read(buffer_, pos_window_, window_size_);
	}

	void write()
	{
		write(buffer_, pos_window_);
	}

	//void shift(const std::streampos old_pos, const std::streampos new_pos)
	//{
	//	std::vector<T> temp_buffer;
	//	read(temp_buffer, old_pos, get_size_file() - old_pos);
	//	warite(temp_buffer, new_pos);
	//}

	//void shift_sliding_window()
	//{
	//	std::streampos offset = pos_window_;

	//	file_.clear();
	//	file_.seekg(offset, std::ios::beg);

	//	std::streamsize current_buffer_len = 0;
	//	std::streamsize new_buffer_len = 0;
	//	for (const auto iter& : buffer_)
	//	{
	//		new_buffer_len += S::get_size_element(iter);
	//		std::streamsize size_element = S::deserialization_size_element(file_);
	//		current_buffer_len += size_element;
	//		file_.seekp(size_element, std::ios::cur);
	//	}
	//	offset += current_buffer_len;

	//	enum class Direction
	//	{
	//		nothing, extention, constriction
	//	};

	//	Direction status;
	//	std::streamsize shift;

	//	if (new_buffer_len > current_buffer_len)
	//	{
	//		status = Direction::extention;
	//		shift = new_buffer_len - current_buffer_len;
	//	}
	//	else if (new_buffer_len < current_buffer_len)
	//	{
	//		status = Direction::constriction;
	//		shift = current_buffer_len - new_buffer_len;
	//	}
	//	else
	//	{
	//		status = Direction::nothing;
	//		shift = 0;
	//	}

	//	std::streamsize file_size = size_file_bytes();

	//	switch (status)
	//	{
	//	case Direction::nothing:
	//	{
	//		write(buffer_, offset);
	//		break;
	//	}

	//	case Direction::extention:
	//	{
	//		std::streamsize current_len = 0;
	//		std::vector<const T> sliding_buffer;

	//		while (offset < file_size)
	//		{


	//			do {
	//				current_len = S::deserialization_size_element(file_);


	//			} while (current_len > shift);

	//			read(sliding_buffer, offset, current_len);

	//			offset += current_len;

	//		}


	//		break;
	//	}

	//	case Direction::constriction:
	//		break;
	//	}
	//}

	//--------------------------------------------------------------------------------------------------------------------------------------//

	std::streampos get_pos_elem(std::streampos start_pos, size_t index_elem)
	{
		const std::streamsize file_size = get_size_file();
		std::streampos offset = start_pos;
		size_t count = 0;

		while (true)
		{
			if (count == index_elem)
			{
				break;
			}
			file_.clear();
			file_.seekg(offset, std::ios::beg);
			std::streamsize size_element = S::deserialization_size_element(file_);
			if (offset + size_element > file_size)
			{
				throw goind_out_of_file();
			}
			offset += size_element;
			count++;
		}
		return offset;
	}

	size_t get_number_elem(const std::streampos start_pos, const std::streamsize range)
	{
		const std::streamsize file_size = get_size_file();
		std::streampos offset = start_pos;
		size_t count = 0;

		while (offset != file_size)
		{
			if (offset > file_size)
			{
				throw goind_out_of_file();
			}
			file_.clear();
			file_.seekg(offset, std::ios::beg);
			std::streamsize size_element = S::deserialization_size_element(file_);
			if (offset + size_element > start_pos + range)
			{
				break;
			}
			offset += size_element;
			count++;
		}
		return count;
	}

	std::streamsize get_size_file() noexcept
	{
		file_.seekg(0, std::ios::end);
		const std::streamsize file_size = file_.tellg();
		file_.seekg(0, std::ios::beg);
		return file_size - offset_end;
	}

	void filling(std::streamsize target_file_size)
	{
		const std::streamsize range = target_file_size - get_size_file();
		T value;
		const std::streamsize size_elem = S::get_size_element(value);
		std::streamsize current_fullness = 0;
		
		while (current_fullness + size_elem <= range)
		{
			this->push_back(value);
			current_fullness += size_elem;
		}
	}

	void liberation(std::streamsize target_file_size)
	{
		while (get_size_file() < target_file_size)
		{
			this->pop_back();
		}
	}
};