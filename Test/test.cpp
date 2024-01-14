#include "pch.h"
#include "VectorFile.hpp"



//template <class T>
//void goo(T&& value)
//{
//	char* ptr = reinterpret_cast<char*>(&value);
//}
//
//template <class T>
//void foo(T&& value)
//{
//	goo(value);
//}

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

	static size_t deserialization_size_element(std::fstream& file)
	{
		size_t size;
		file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
		return sizeof(size_t) + size * sizeof(T);
	}

	static size_t get_size_element(const std::vector<T>& elem)
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

int main()
{
	auto p = std::filesystem::temp_directory_path() / "temp.bin";
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<size_t>(0));

		vec.push_back({ 1, 5, 6 });
		vec.push_back({ 5, 6 });
		vec.push_back({ 0 });
		std::cout << vec.size() << "\n" << vec.file_len() << "\n" << std::endl;

		vec.pop_back();
		std::cout << vec.size() << "\n" << vec.file_len() << "\n" << std::endl;

		vec.pop_back();
		std::cout << vec.size() << "\n" << vec.file_len() << "\n" << std::endl;

		vec.pop_back();
		std::cout << vec.size() << "\n" << vec.file_len() << "\n" << std::endl;

		vec.push_back({ 0, 0, 0, 0, 0 });
		vec.push_back({ 0, 0, 0 });
		vec.push_back({ 0 });
		vec.push_back({ 0, 0, 0, 0, 0, 0, 0, 0 });

		for (int i = 0; i < 100; i++)
		{
			vec.push_back({ 0, 0, 0, 0, 0, 0, 0, 0 });
			vec.push_back({ 0, 0, 0 });
			vec.push_back({ 0, 0, 0, 0, 0, 0 });
		}

		std::cout << vec.size() << "\n" << vec.file_len() << "\n" << std::endl;
		print_vector_file(vec);

		vec[0] = { 1, 2, 3, 4, 5 };
		vec[3] = { 1, 3, 5, 7, 9, 11, 13, 15 };
		print_vector_file(vec);
		vec.flush();
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p, static_cast<bool>(true));
		for (int i = 0; i < 300; i++)
		{
			vec.pop_back();
		}
		print_vector_file(vec);
	}
	{
		VectorFile<std::vector<int>, SerializerVector<int>> vec(p);
		print_vector_file(vec);
	}
	std::filesystem::remove(p);
	return 0;
}






TEST(CreateAndOpenFiles, Test2)
{
	auto p = std::filesystem::current_path() / "temp.bin";
	{
		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
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





//#include "unordered_map"
//
//
//class MyType final
//{
//	int arg1_;
//	double arg2_;
//	char arg3_;
//
//public:
//	MyType() = default;
//	MyType(int arg1, double arg2, char arg3) : arg1_(arg1), arg2_(arg2), arg3_(arg3) { }
//	MyType(const MyType&) = default;
//	MyType& operator=(const MyType&) = default;
//	MyType(MyType&&) noexcept = default;
//	MyType& operator=(MyType&&) noexcept = default;
//	bool operator==(const MyType&) const = default;
//	~MyType() = default;
//};
//
//
//TEST(OpenFiles, Read)
//{
//	//std::unordered_map<>
//
//
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(32));
//		EXPECT_EQ(vec.size_file(), 32);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 32);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_NO_THROW(vec[i]);
//		}
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_NO_THROW(vec.seek_window(i));
//		}
//
//		vec[0] = 5;
//		EXPECT_THROW(vec.flush(), write_error);
//	}
//	std::filesystem::remove(p);
//}
//
//TEST(OpenFiles, Write)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(32));
//		EXPECT_EQ(vec.size_file(), 32);
//	}
//	{
//		VectorFile<uint8_t> vec(p, true);
//		EXPECT_EQ(vec.size_file(), 32);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_NO_THROW(vec[i] = static_cast<uint8_t>(i));
//		}
//		vec.flush();
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i);
//		}
//	}
//	std::filesystem::remove(p);
//}
//
//TEST(OpenFiles, Test_MyType)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	size_t size_type = sizeof(MyType);
//	size_t size_file = size_type * 10;
//	{
//		VectorFile<MyType> vec(p, size_file);
//		EXPECT_EQ(vec.size_file(), size_file);
//		for (size_t i = 0; i < vec.size_file() / size_file; i++)
//		{
//			vec[i] = MyType(static_cast<int>(i), 3.4, 'a');
//		}
//	}
//	{
//		VectorFile<MyType> vec(p);
//		EXPECT_EQ(vec.size_file(), size_file);
//		for (size_t i = 0; i < vec.size_file() / size_file; i++)
//		{
//			EXPECT_NO_THROW(vec[i]);
//		}
//		for (size_t i = 0; i < vec.size_file() / size_file; i++)
//		{
//			EXPECT_NO_THROW(vec.seek_window(i));
//		}
//
//		vec[0] = MyType(20, 3.4, 'a');
//		EXPECT_THROW(vec.flush(), write_error);
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(CreateAndOpenFiles, Test1)
//{
//	auto p = std::filesystem::current_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//}
//
//TEST(CreateAndOpenFiles, Test2)
//{
//	auto p = std::filesystem::current_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i + 100);
//		}
//		vec.flush();
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 64);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i + 100);
//		}
//	}
//}
//
//TEST(CreateFiles, Empty)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(0));
//		EXPECT_EQ(vec.size_file(), 0);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 0);
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(PushBack, CreateFile_Init)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(8), 4);
//
//		EXPECT_EQ(vec.size_file(), 8);
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i];
//		}
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i + 60);
//		}
//		vec.flush();
//
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i + 60);
//		}
//
//		for (size_t i = 0; i < 10; i++)
//		{
//			vec.push_back(60);
//		}
//
//		EXPECT_EQ(vec.size_file(), 18);
//
//		for (size_t i = 8; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], 60);
//		}
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(PushBack, CreateFile_Init_Check)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(8), 4);
//
//		EXPECT_EQ(vec.size_file(), 8);
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i);
//		}
//
//		for (size_t i = 0; i < 10; i++)
//		{
//			vec.push_back(static_cast<uint8_t>(8 + i));
//		}
//
//		EXPECT_EQ(vec.size_file(), 18);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 18);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i);
//		}
//	}
//
//	std::filesystem::remove(p);
//}
//
//TEST(PushBack, CreateFile_Uninit)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(8), 4);
//
//		EXPECT_EQ(vec.size_file(), 8);
//
//		for (uint8_t i = 0; i < 10; i++)
//		{
//			vec.push_back(60);
//		}
//
//		EXPECT_EQ(vec.size_file(), 18);
//
//		for (uint8_t i = 0; i < 8; i++)
//		{
//			vec[i] = i + 60;
//		}
//		vec.flush();
//
//		for (uint8_t i = 0; i < 8; i++)
//		{
//			EXPECT_EQ(vec[i], i + 60);
//		}
//
//		for (uint8_t i = 8; i < 18; i++)
//		{
//			EXPECT_EQ(vec[i], 60);
//		}
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(PushBack, OneElem)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(1024), 128);
//		EXPECT_EQ(vec.size_file(), 1024);
//		vec.push_back(145);
//		EXPECT_EQ(vec.size_file(), 1024 + 1);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 1024 + 1);
//		EXPECT_EQ(vec[1024], 145);
//	}
//
//	std::filesystem::remove(p);
//}
//
//TEST(PopBack, SmalWindow)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(16), 4);
//		for (uint8_t i = 0; i < 8; i++)
//		{
//			vec.pop_back();
//		}
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 8);
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(PopBack, BigWindow)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(16), 64);
//		for (uint8_t i = 0; i < 8; i++)
//		{
//			vec.pop_back();
//		}
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 8);
//	}
//	std::filesystem::remove(p);
//}
//
//TEST(PopBack, EmptyFile)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(0));
//		EXPECT_THROW(vec.pop_back(), goind_out_of_file);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 0);
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(Resize, Decrease)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		EXPECT_EQ(vec.size_file(), 64);
//		vec.resize(32);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 32);
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(Resize, Increase)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		EXPECT_EQ(vec.size_file(), 64);
//		vec.resize(128);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		EXPECT_EQ(vec.size_file(), 128);
//	}
//	std::filesystem::remove(p);
//}
//
//
//
//TEST(Move, Test1)
//{
//	auto p = std::filesystem::current_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i + 100);
//		}
//		vec.flush();
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec(p);
//		VectorFile<uint8_t> vec_new(std::move(vec));
//		EXPECT_EQ(vec_new.size_file(), 64);
//	}
//}
//
//
//TEST(Move, Test2)
//{
//	auto p = std::filesystem::current_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i + 100);
//		}
//		vec.flush();
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec_new(VectorFile<uint8_t>{p});
//		EXPECT_EQ(vec_new.size_file(), 64);
//	}
//}
//
//
//TEST(Move, Test3)
//{
//	auto p = std::filesystem::current_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i + 100);
//		}
//		vec.flush();
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec_new(VectorFile<uint8_t>{p});
//		VectorFile<uint8_t> vec_new2(VectorFile<uint8_t>{p});
//		vec_new2 = std::move(vec_new);
//		EXPECT_EQ(vec_new.size_file(), 64);
//	}
//}
//
//
//TEST(AllFunctionality, _)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64), 16);
//		EXPECT_EQ(vec.size_file(), 64);
//		vec.push_back(0);
//		EXPECT_EQ(vec.size_file(), 65);
//		vec.pop_back();
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec(p, true, 32);
//		EXPECT_EQ(vec.size_file(), 64);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i);
//		}
//		vec.flush();
//	}
//	{
//		VectorFile<uint8_t> vec(p, false, 8);
//		EXPECT_EQ(vec.size_file(), 64);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i);
//		}
//	}
//	{
//		VectorFile<uint8_t> vec(p, true, 16);
//		EXPECT_EQ(vec.size_file(), 64);
//		vec.seek_window(63);
//		for (size_t i = 0; i < 32; i++)
//		{
//			vec.push_back(64 + static_cast<uint8_t>(i));
//		}
//		EXPECT_EQ(vec.size_file(), 96);
//	}
//	{
//		VectorFile<uint8_t> vec(p, false, 24);
//		EXPECT_EQ(vec.size_file(), 96);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i);
//		}
//	}
//	{
//		VectorFile<uint8_t> vec(p, true, 4);
//		EXPECT_EQ(vec.size_file(), 96);
//		vec.seek_window(63);
//		for (size_t i = 95; i > 63; i--)
//		{
//			EXPECT_EQ(vec.pop_back(), i);
//		}
//		EXPECT_EQ(vec.size_file(), 64);
//	}
//	{
//		VectorFile<uint8_t> vec(p, false, 24);
//		EXPECT_EQ(vec.size_file(), 64);
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i);
//		}
//	}
//	std::filesystem::remove(p);
//}
//
//
//TEST(Iteration, _)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//
//		EXPECT_EQ(vec.size_file(), 64);
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i);
//		}
//		vec.flush();
//
//
//		int i = 0;
//		for (auto iter: vec)
//		{
//			EXPECT_EQ(iter, i);
//			i++;
//		}
//	}
//
//	std::filesystem::remove(p);
//}
//
//TEST(Shift, _)
//{
//	auto p = std::filesystem::temp_directory_path() / "temp.bin";
//	{
//		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
//
//		EXPECT_EQ(vec.size_file(), 64);
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			vec[i] = static_cast<uint8_t>(i);
//		}
//		vec.flush();
//
//		vec.shift(0, 32);
//
//		EXPECT_EQ(vec.size_file(), 96);
//
//		for (size_t i = 0; i < 32; i++)
//		{
//			vec[i] = static_cast<uint8_t>(i);
//		}
//		vec.flush();
//
//
//		for (size_t i = 0; i < vec.size_file(); i++)
//		{
//			EXPECT_EQ(vec[i], i);
//		}
//
//		//int i = 0;
//		//for (auto iter : vec)
//		//{
//		//	EXPECT_EQ(iter, i);
//		//	i++;
//		//}
//	}
//
//	std::filesystem::remove(p);
//}
//
////TEST(Iterator, Test1)
////{
////	auto p = std::filesystem::current_path() / "temp.bin";
////	{
////		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
////		vec.seek_window(0);
////		for (std::vector<uint8_t>::iterator iter : vec)
////		{
////			iter = 10;
////		}
////		vec.flush();
////		EXPECT_EQ(vec.size_file(), 64);
////	}
////	{
////		VectorFile<uint8_t> vec(p);
////		EXPECT_EQ(vec.size_file(), 64);
////		vec.seek_window(0);
////		for (auto iter : vec)
////		{
////			EXPECT_EQ(iter, 10);
////		}
////	}
////}
//
//
////TEST(Iterator, Test1)
////{
////	auto p = std::filesystem::current_path() / "temp.bin";
////	{
////		VectorFile<uint8_t> vec(p, static_cast<size_t>(64));
////		for (size_t i = 0; i < vec.size_file(); i++)
////		{
////			vec[i] = static_cast<uint8_t>(i);
////		}
////		vec.flush();
////		EXPECT_EQ(vec.size_file(), 64);
////	}
////	{
////		VectorFile<uint8_t> vec(p, false);
////		EXPECT_EQ(vec.size_file(), 64);
////		for (size_t i = 0; i < vec.size_file(); i++)
////		{
////			EXPECT_EQ(vec[i], i);
////		}
////		size_t i = 0;
////		for (auto iter : vec)
////		{
////			EXPECT_EQ(iter, i);
////			i++;
////		}
////	}
////}