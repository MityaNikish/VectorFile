#pragma once
#include <fstream>
#include <utility>
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

	static std::streamsize deserialization_size_element(std::fstream& file)
	{
		return sizeof(T);
	}

	static std::streamsize get_size_element(const T& elem)
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
	bool is_writable_;						//флаг чтение-запись/чтение
	std::fstream file_;						//файл
	std::filesystem::path path_;			//путь к файлу
	std::streamsize window_size_;			//ширина окна (байт)
	std::streampos pos_window_;				//смещение окна от начала (байт)
	std::streamoff offset_end;			    //смещение с конца(байт)
	std::vector<T> buffer_;					//буфер элиментов окна

public:
	explicit VectorFile(std::filesystem::path path, bool is_write = false, size_t window_size = 1024)
		: is_writable_(is_write), path_(std::move(path)), window_size_(window_size), pos_window_(0), offset_end(0)
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
		: is_writable_(true), path_(std::move(path)), window_size_(window_size), pos_window_(0), offset_end(0)
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
		if (is_writable_)
		{
			write();
		}

		const std::streamsize file_size = get_size_file();

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
			write();
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
		write();
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

	enum class Direction
	{
		nothing, extention, constriction
	};

	void read()
	{
		buffer_.clear();
		const std::streamsize file_size = get_size_file();
		std::streampos offset = pos_window_;

		while (true)
		{
			if (offset == file_size)
			{
				break;
			}
			file_.clear();
			file_.seekg(offset, std::ios::beg);
			const std::streamsize elem_size = S::deserialization_size_element(file_);
			if (offset + elem_size > pos_window_ + window_size_)
			{
				break;
			}
			T obj;
			file_.clear();
			file_.seekg(offset, std::ios::beg);
			S::deserialization(file_, obj);
			buffer_.push_back(obj);
			offset += elem_size;
		}
	}

	void write()
	{
		std::streampos current_offset = pos_window_;
		std::streamsize current_buffer_len = 0;
		std::streamsize new_buffer_len = 0;
		for (auto iter : buffer_)
		{
			new_buffer_len += S::get_size_element(iter);

			file_.clear();
			file_.seekp(current_offset, std::ios::beg);
			const std::streamsize elem_size = S::deserialization_size_element(file_) ;

			current_buffer_len += elem_size;
			current_offset += elem_size;
		}

		Direction status;
		std::streamsize shift;
		shift_determination(current_buffer_len, new_buffer_len, shift, status);

		const std::streamsize file_size = get_size_file();

		switch (status)
		{
			case Direction::nothing:
			{
				write_buffer();
				break;
			}

			case Direction::extention:
			{
				std::vector<T> sliding_buffer;

				eclipse_buffering(shift, file_size, current_offset, sliding_buffer);

				write_buffer();
				std::streampos offset = pos_window_ + new_buffer_len;

				while (offset - shift != file_size)
				{
					for (size_t i = 0; i < sliding_buffer.size(); i++)
					{
						file_.clear();
						file_.seekp(offset, std::ios::beg);
						S::serialization(file_, sliding_buffer[i]);
						const std::streamsize elem_size = S::get_size_element(sliding_buffer[i]);
						offset += elem_size;
					}
					sliding_buffer.clear();
					eclipse_buffering(shift, file_size, current_offset, sliding_buffer);
				}

				offset_end = offset_end < shift ? 0 : offset_end - shift;
				break;
			}

			case Direction::constriction:
			{
				std::streampos offset = pos_window_;
				for (size_t i = 0; i < buffer_.size(); i++)
				{
					file_.clear();
					file_.seekp(offset, std::ios::beg);
					S::serialization(file_, buffer_[i]);
					const std::streamsize elem_size = S::get_size_element(buffer_[i]);
					offset += elem_size;
				}
				while (current_offset != file_size)
				{
					T obj;

					file_.clear();
					file_.seekp(current_offset, std::ios::beg);
					S::deserialization(file_, obj);

					file_.clear();
					file_.seekp(offset, std::ios::beg);
					S::serialization(file_, obj);

					const std::streamsize elem_size = S::get_size_element(obj);
					offset += elem_size;
					current_offset += elem_size;
				}
				offset_end += shift;
				break;
			}
		}
	}

	void shift_determination(const std::streamsize& cur_len, const std::streamsize& new_len, std::streamsize& shift, Direction& status)
	{
		if (new_len > cur_len)
		{
			status = Direction::extention;
			shift = new_len - cur_len;
		}
		else if (new_len < cur_len)
		{
			status = Direction::constriction;
			shift = cur_len - new_len;
		}
		else
		{
			status = Direction::nothing;
			shift = 0;
		}
	}

	void write_buffer()
	{
		std::streampos offset = pos_window_;
		for (size_t i = 0; i < buffer_.size(); i++)
		{
			file_.clear();
			file_.seekp(offset, std::ios::beg);
			S::serialization(file_, buffer_[i]);
			const std::streamsize elem_size = S::get_size_element(buffer_[i]);
			offset += elem_size;
		}
	}

	void eclipse_buffering(const std::streamsize& shift, const std::streamsize& file_size, std::streampos& current_offset, std::vector<T>& sliding_buffer)
	{
		while (current_offset != file_size)
		{
			T obj;
			file_.clear();
			file_.seekp(current_offset, std::ios::beg);
			S::deserialization(file_, obj);
			sliding_buffer.push_back(obj);
			const std::streamsize elem_size = S::get_size_element(obj);
			if (shift < elem_size)
			{
				break;
			}
			current_offset += elem_size;
		}
	}

	std::streampos get_pos_elem(const std::streampos& start_pos, size_t index_elem)
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

	size_t get_number_elem(const std::streampos& start_pos, const std::streamsize& range)
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
			const std::streamsize size_element = S::deserialization_size_element(file_);
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

	void filling(const std::streamsize& target_file_size)
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

	void liberation(const std::streamsize& target_file_size)
	{
		while (get_size_file() < target_file_size)
		{
			this->pop_back();
		}
	}
};