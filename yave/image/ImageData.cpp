/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "ImageData.h"

#include <y/io/BuffReader.h>
#include <y/io/Decoder.h>

namespace yave {


ImageData::~ImageData() {
	delete[] _data;
}

ImageData::ImageData(ImageData&& other) : ImageData() {
	swap(other);
}

ImageData& ImageData::operator=(ImageData&& other) {
	swap(other);
	return *this;
}

usize ImageData::byte_size() const {
	return _size.x() *_size.y() *_bpp;
}

const math::Vec2ui& ImageData::size() const {
	return _size;
}

const u8* ImageData::raw_pixel(const math::Vec2ui& pos) {
	return _data +
			((_size.x() *_bpp) *pos.x()) +
			(_bpp *pos.y())
		;
}

const u8* ImageData::raw_data() const {
	return _data;
}

void ImageData::swap(ImageData& other) {
	std::swap(_size, other._size);
	std::swap(_bpp, other._bpp);
	std::swap(_data, other._data);
}

ImageData ImageData::from_file(io::ReaderRef reader) {
	auto decoder = io::Decoder(reader);

	u32 height = 0;
	u32 width = 0;
	u64 data_size = 0;

	decoder.decode<io::Byteorder::BigEndian>(width);
	decoder.decode<io::Byteorder::BigEndian>(height);
	decoder.decode<io::Byteorder::BigEndian>(data_size);

	if(height * width * 4 != data_size) {
		fatal("Invalid file size.");
	}

	ImageData data;
	data._bpp = 4;
	data._size = math::vec(width, height);
	data._data = new u8[data_size];

	if(reader->read(data._data, data_size) != data_size) {
		fatal("Invalid file size.");
	}

	return data;
}

}
