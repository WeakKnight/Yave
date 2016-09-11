#include <iostream>
#include <y/io/File.h>
#include <y/io/BuffReader.h>
#include <y/io/Decoder.h>

using namespace y;
using namespace io;

void print_file(ReaderRef file) {
	usize data_size = 10;
	char *data = new char[data_size];
	while(!file->at_end()) {
		usize read = file->read(data, data_size - 1);
		data[read] = 0;
		std::cout << data;
	}
	std::cout << std::endl;
}

using Vertex = std::array<float, 8>;
using IndexedTriangle = std::array<u32, 3>;

struct MeshData {
	core::Vector<Vertex> vertices;
	core::Vector<IndexedTriangle> triangles;
};

MeshData test(ReaderRef reader) {
	auto decoder = Decoder(BuffReader(reader));

	u32 magic = 0;
	u64 version = 0;
	MeshData mesh;

	decoder.decode<io::Byteorder::BigEndian>(magic);
	decoder.decode<io::Byteorder::BigEndian>(version);

	if(magic != 0x79617665 || version != 1) {
		return mesh;
	}

	decoder.decode<io::Byteorder::BigEndian>(mesh.vertices);
	decoder.decode<io::Byteorder::BigEndian>(mesh.triangles);

	return mesh;
}

int main(int, char **) {
	auto file = File::open("../../yave/tools/mesh/chalet.ym");

	auto mesh = test(file);
	std::cout << mesh.triangles.size() << " triangles loaded " << std::endl;
	return 0;
}
