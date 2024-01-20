#include "pch.h"
#include "vector_file.hpp"


template <typename T>
struct SerializerVector
{
	static void serialization(std::fstream& file, const std::vector<T>& elem)
	{
		size_t size = elem.size();
		file.write(reinterpret_cast<char*>(&size), sizeof(size_t));
		for (const T& item : elem) {
			file.write(reinterpret_cast<const char*>(&item), sizeof(T));
		}
	}

	static void deserialization(std::fstream& file, std::vector<T>& elem)
	{
		size_t size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
		elem.resize(size);
		for (T& item : elem) {
			file.read(reinterpret_cast<char*>(&item), sizeof(T));
		}
	}

	static std::streamsize deserialization_size_element(std::fstream& file)
	{
		size_t size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
		return sizeof(size_t) + size * sizeof(T);
	}

	static std::streamsize get_size_element(const std::vector<T>& elem)
	{
		return sizeof(size_t) + elem.size() * sizeof(T);
	}
};

template<typename T, typename S>
void print_vector_file(VectorFile<T, S>& vec)
{
	for (int i = 0; i < vec.size(); i++)
	{
		T value = vec[i];
		for (auto iter : value)
		{
			std::cout << iter << "  ";
		}
		std::cout << "\n";
	}
	std::cout << "\n\n" << std::endl;
}


TEST(OpenFiles, Const)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<const uint8_t> vec(p, static_cast<std::streamsize>(0));
		for (size_t i = 0; i < 100; i++)
		{
			vec.push_back(static_cast<uint8_t>(i));
		}
		vec.flush();
	}
	{
		VectorFile<const uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 100);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_NO_THROW(vec[i]);
		}
	}
	std::filesystem::remove(p);
}


TEST(DynamicStruct, Push_Pop_Back)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(0));

		vec.push_back({ 1, 5, 6 });	//20
		vec.push_back({ 5, 6 });	//16
		vec.push_back({ 0 });		//12
		EXPECT_EQ(vec.size(), 3);
		EXPECT_EQ(vec.file_len(), 48);

		vec.pop_back();
		EXPECT_EQ(vec.size(), 2);
		EXPECT_EQ(vec.file_len(), 36);

		vec.pop_back();
		EXPECT_EQ(vec.size(), 1);
		EXPECT_EQ(vec.file_len(), 20);

		vec.pop_back();
		EXPECT_EQ(vec.size(), 0);
		EXPECT_EQ(vec.file_len(), 0);
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);
		EXPECT_EQ(vec.size(), 0);
		EXPECT_EQ(vec.file_len(), 0);
	}
	std::filesystem::remove(p);
}


TEST(DynamicStruct, Shift)
{
	size_t size;
	std::streamsize file_size;
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(500));

		for (auto i = 0; i < vec.size(); i++)
		{
			vec[i].push_back(10);
			vec[i].push_back(7);
		}
		vec.flush();
		vec.pop_back();
		size = vec.size();
		file_size = vec.file_len();
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);

		EXPECT_EQ(vec.size(), size);
		EXPECT_EQ(vec.file_len(), file_size);
	}
	std::filesystem::remove(p);
}


TEST(DynamicStruct, Push_Pop_Back_More)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(0));

		for (int i = 0; i < 100; i++)
		{
			vec.push_back({ 0, 0, 0, 0, 0, 0, 0, 0 });
			vec.push_back({ 0, 0, 0 });
			vec.push_back({ 0, 0, 0, 0, 0, 0 });
		}
		EXPECT_EQ(vec.size(), 300);

		vec[0] = { 1, 2, 3, 4, 5 };
		vec[1] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
		vec[2] = { 1, 3, 5, 7, 9, 11, 13, 15, 25 };
		vec.flush();
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<bool>(true));

		EXPECT_EQ(vec[0], std::vector<int>({ 1, 2, 3, 4, 5 }));
		EXPECT_EQ(vec[1], std::vector<int>({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }));
		EXPECT_EQ(vec[2], std::vector<int>({ 1, 3, 5, 7, 9, 11, 13, 15, 25 }));

		for (int i = 0; i < 300; i++)
		{
			vec.pop_back();
		}
		EXPECT_EQ(vec.size(), 0);
		EXPECT_EQ(vec.file_len(), 0);
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);
		EXPECT_EQ(vec.size(), 0);
		EXPECT_EQ(vec.file_len(), 0);
	}
	std::filesystem::remove(p);
}


TEST(OpenFiles, Read)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(128), 20);
		EXPECT_EQ(vec.file_len(), 128);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_NO_THROW(vec[i] = static_cast<uint8_t>(i));
		}
		vec.flush();
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 128);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_NO_THROW(vec[i]);
		}
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_NO_THROW(vec.seek_window(i));
		}

		vec[0] = 5;
		EXPECT_THROW(vec.flush(), write_error);
	}
	std::filesystem::remove(p);
}


TEST(OpenFiles, Write)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(128), 20);
		EXPECT_EQ(vec.file_len(), 128);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_NO_THROW(vec[i] = static_cast<uint8_t>(i));
		}
		vec.flush();
	}
	{
		VectorFile<uint8_t> vec(p, true);
		EXPECT_EQ(vec.file_len(), 128);

		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i);
		}
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_NO_THROW(vec[i] = static_cast<uint8_t>(0));
		}
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 128);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], 0);
		}
	}
	std::filesystem::remove(p);
}


TEST(CreateAndOpenFiles, Test1)
{
	auto p = std::filesystem::current_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));
		EXPECT_EQ(vec.size(), 64);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.size(), 64);
	}
}


TEST(CreateAndOpenFiles, Test2)
{
	auto p = std::filesystem::current_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));
		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i + 100);
		}
		vec.flush();
		EXPECT_EQ(vec.file_len(), 64);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 64);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i + 100);
		}
	}
}


TEST(CreateFiles, Empty)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(0));
		EXPECT_EQ(vec.file_len(), 0);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 0);
	}
	std::filesystem::remove(p);
}


TEST(PushBack, CreateFile_Init)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(30), 4);

		EXPECT_EQ(vec.file_len(), 30);

		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i + 60);
		}
		vec.flush();


		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i + 60);
		}

		for (size_t i = 0; i < 10; i++)
		{
			vec.push_back(60);
		}

		EXPECT_EQ(vec.file_len(), 40);

		for (size_t i = 30; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], 60);
		}
	}
	std::filesystem::remove(p);
}


TEST(PushBack, CreateFile_Init_Check)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(8), 4);

		EXPECT_EQ(vec.file_len(), 8);

		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i);
		}

		for (size_t i = 0; i < 10; i++)
		{
			vec.push_back(static_cast<uint8_t>(8 + i));
		}

		EXPECT_EQ(vec.file_len(), 18);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 18);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i);
		}
	}
	std::filesystem::remove(p);
}


TEST(PushBack, CreateFile_Uninit)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(8), 4);

		EXPECT_EQ(vec.file_len(), 8);

		for (uint8_t i = 0; i < 10; i++)
		{
			vec.push_back(60);
		}

		EXPECT_EQ(vec.file_len(), 18);

		for (uint8_t i = 0; i < 8; i++)
		{
			vec[i] = i + 60;
		}
		vec.flush();

		for (uint8_t i = 0; i < 8; i++)
		{
			EXPECT_EQ(vec[i], i + 60);
		}

		for (uint8_t i = 8; i < 18; i++)
		{
			EXPECT_EQ(vec[i], 60);
		}
	}
	std::filesystem::remove(p);
}


TEST(PushBack, OneElem)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(1024), 128);
		EXPECT_EQ(vec.file_len(), 1024);
		vec.push_back(145);
		EXPECT_EQ(vec.file_len(), 1024 + 1);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 1024 + 1);
		EXPECT_EQ(vec[1024], 145);
	}
	std::filesystem::remove(p);
}


TEST(PopBack, SmalWindow)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(16), 4);
		vec[14];
		for (uint8_t i = 0; i < 8; i++)
		{
			vec.pop_back();
		}
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 8);
	}
	std::filesystem::remove(p);
}


TEST(PopBack, BigWindow)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(16), 64);
		for (uint8_t i = 0; i < 8; i++)
		{
			vec.pop_back();
		}
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 8);
	}
	std::filesystem::remove(p);
}


TEST(PopBack, EmptyFile)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(0));
		EXPECT_TRUE(vec.empty());
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 0);
	}
	std::filesystem::remove(p);
}


TEST(Resize, Decrease_Simple)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));
		EXPECT_EQ(vec.file_len(), 64);
		vec.resize(32);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 32);
	}
	std::filesystem::remove(p);
}


TEST(Resize, Decrease_Structure_1)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(64));
		EXPECT_EQ(vec.file_len(), 64);
		vec[0] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; //48
		vec.flush();
		vec.resize(32);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_TRUE(vec.empty());
	}
	std::filesystem::remove(p);
}


TEST(Resize, Decrease_Structure_2)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(0));
		EXPECT_TRUE(vec.empty());

		for (int i = 0; i < 3; i++)						//60 * 3
		{
			vec.push_back({ 1 });				//12
			vec.push_back({ 3, 5, 7 });			//20
			vec.push_back({ 5, 7, 9, 11, 13 });	//28
		}
		EXPECT_EQ(vec.size(), 9);
		vec.resize(60);
		EXPECT_EQ(vec.size(), 3);
		EXPECT_EQ(vec.file_len(), 60);
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);
		EXPECT_EQ(vec.size(), 3);
		EXPECT_EQ(vec.file_len(), 60);
	}
	std::filesystem::remove(p);
}


TEST(Resize, Increase_Simple)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));
		EXPECT_EQ(vec.file_len(), 64);
		vec.resize(128);
	}
	{
		VectorFile<uint8_t> vec(p);
		EXPECT_EQ(vec.file_len(), 128);
	}
	std::filesystem::remove(p);
}


TEST(Resize, Increase_Structure_1)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(0));
		EXPECT_TRUE(vec.empty());

		vec.resize(32);
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);
		EXPECT_EQ(vec.size(), 4);
	}
	std::filesystem::remove(p);
}


TEST(Resize, Increase_Structure_2)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<std::streamsize>(0));
		vec.push_back({1, 2, 3, 4, 5, 6, 7, 8});	//40
		EXPECT_EQ(vec.size(), 1);
		vec.resize(45);
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);
		EXPECT_EQ(vec.size(), 1);
	}
	std::filesystem::remove(p);
}


TEST(Move, Test1)
{
	auto p = std::filesystem::current_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));
		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i + 100);
		}
		vec.flush();
		EXPECT_EQ(vec.file_len(), 64);
	}
	{
		VectorFile<uint8_t> vec(p);
		VectorFile<uint8_t> vec_new(std::move(vec));
		for (size_t i = 0; i < vec_new.size(); i++)
		{
			EXPECT_EQ(vec_new[i], i + 100);
		}
		EXPECT_EQ(vec_new.file_len(), 64);
	}
}


TEST(Move, Test2)
{
	auto p = std::filesystem::current_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));
		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i + 100);
		}
		vec.flush();
		EXPECT_EQ(vec.file_len(), 64);
	}
	{
		VectorFile<uint8_t> vec_new(VectorFile<uint8_t>{p});
		for (size_t i = 0; i < vec_new.size(); i++)
		{
			EXPECT_EQ(vec_new[i], i + 100);
		}
		EXPECT_EQ(vec_new.file_len(), 64);
	}
}


TEST(AllFunctionality, _)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64), 16);
		EXPECT_EQ(vec.file_len(), 64);
		vec.push_back(0);
		EXPECT_EQ(vec.file_len(), 65);
		vec.pop_back();
		EXPECT_EQ(vec.file_len(), 64);
	}
	{
		VectorFile<uint8_t> vec(p, true, 32);
		EXPECT_EQ(vec.file_len(), 64);
		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i);
		}
		vec.flush();
	}
	{
		VectorFile<uint8_t> vec(p, false, 8);
		EXPECT_EQ(vec.file_len(), 64);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i);
		}
	}
	{
		VectorFile<uint8_t> vec(p, true, 16);
		EXPECT_EQ(vec.file_len(), 64);
		vec.seek_window(63);
		for (size_t i = 0; i < 32; i++)
		{
			vec.push_back(64 + static_cast<uint8_t>(i));
		}
		EXPECT_EQ(vec.file_len(), 96);
	}
	{
		VectorFile<uint8_t> vec(p, false, 24);
		EXPECT_EQ(vec.file_len(), 96);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i);
		}
	}
	{
		VectorFile<uint8_t> vec(p, true, 4);
		EXPECT_EQ(vec.file_len(), 96);
		vec.seek_window(63);
		for (size_t i = 95; i > 63; i--)
		{
			EXPECT_EQ(vec.pop_back(), i);
		}
		EXPECT_EQ(vec.file_len(), 64);
	}
	{
		VectorFile<uint8_t> vec(p, false, 24);
		EXPECT_EQ(vec.file_len(), 64);
		for (size_t i = 0; i < vec.size(); i++)
		{
			EXPECT_EQ(vec[i], i);
		}
	}
	std::filesystem::remove(p);
}


TEST(Iteration, _)
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<std::streamsize>(64));

		EXPECT_EQ(vec.file_len(), 64);

		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = static_cast<uint8_t>(i);
		}
		vec.flush();


		int i = 0;
		for (auto iter: vec)
		{
			EXPECT_EQ(iter, i);
			i++;
		}
	}

	std::filesystem::remove(p);
}