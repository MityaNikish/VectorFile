#include "VectorFile.hpp"

template <Acceptable T, class S = Serializer<T>>
VectorFile<T, S>::VectorFile(std::filesystem::path path, bool is_write, size_t window_size)
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

template <Acceptable T, class S = Serializer<T>>
VectorFile<T, S>::VectorFile(std::filesystem::path path, size_t file_size, size_t window_size)
	: is_write_(true), path_(std::move(path)), file_size_(0), target_window_size_(window_size), offset_window_(0)
{
	target_file_size_ = align_filesize_to_typesize(file_size);

	file_.open(path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

	const size_t count_elem_for_filling = target_file_size_ > target_window_size_ ? target_window_size_ / type_size_ : target_file_size_ / type_size_;
	filling(count_elem_for_filling);
	read();
}

template <Acceptable T, class S = Serializer<T>>
VectorFile<T, S>::~VectorFile()
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

template <Acceptable T, class S = Serializer<T>>
size_t VectorFile<T, S>::size_buffer() const noexcept
{
	return buffer_.size();
}

template <Acceptable T, class S = Serializer<T>>
size_t VectorFile<T, S>::size_file() const noexcept
{
	return target_file_size_;
}

template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::seek_window(size_t amount_elements)
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

template <Acceptable T, class S = Serializer<T>>
T& VectorFile<T, S>::operator[](size_t index)
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

template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::flush()
{
	if (!is_write_)
	{
		throw write_error();
	}
	write();
}

template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::push_back(const T& value)
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

template <Acceptable T, class S = Serializer<T>>
T VectorFile<T, S>::pop_back() {
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

template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::resize(size_t new_file_size)
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


template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::read()
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

template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::write()
{
	file_.clear();
	file_.seekp(offset_window_, std::ios::beg);
	for (size_t i = 0; i < buffer_.size(); i++)
	{
		S::serialization(file_, buffer_[i]);
		//file_.write(reinterpret_cast<char*>(&buffer_[i]), type_size_);
	}
}

template <Acceptable T, class S = Serializer<T>>
size_t VectorFile<T, S>::get_size_file()
{
	file_.seekg(0, std::ios::end);
	const size_t file_size = file_.tellg();
	file_.seekg(0, std::ios::beg);
	return file_size;
}

template <Acceptable T, class S = Serializer<T>>
void VectorFile<T, S>::filling(const size_t number_elem)
{
	file_.clear();
	file_.seekp(0, std::ios::end);

	for (size_t i = 0; i < number_elem * type_size_; i++)
	{
		file_.write("\0", 1);
	}
	file_size_ += number_elem * type_size_;
}

template <Acceptable T, class S = Serializer<T>>
size_t VectorFile<T, S>::align_filesize_to_typesize(size_t original_file_size) const
{
	return original_file_size / type_size_ * type_size_;
}