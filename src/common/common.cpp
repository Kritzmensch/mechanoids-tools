/*
 * AIM tools
 * Copyright (C) 2015 lzwdgc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"

#include <stdio.h>

const int build_version =
#include <version.h.in>
;

std::string version()
{
    using namespace std;

    string s;
    s = to_string(0) + "." +
        to_string(1) + "." +
        to_string(0) + "." +
        to_string(build_version);
    return s;
}

std::vector<uint8_t> readFile(const std::string &fn)
{
    FILE *f = fopen(fn.c_str(), "rb");
    if (!f)
        throw std::runtime_error("Cannot open file " + fn);
    fseek(f, 0, SEEK_END);
    auto sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(sz);
    fread(buf.data(), 1, sz, f);
    fclose(f);
    return buf;
}

void writeFile(const std::string &fn, const std::vector<uint8_t> &data)
{
    FILE *f = fopen(fn.c_str(), "wb");
    if (!f)
        throw std::runtime_error("Cannot open file " + fn);
    fwrite(data.data(), 1, data.size(), f);
    fclose(f);
}

buffer::buffer()
{
}

buffer::buffer(size_t size)
    : buf(new std::vector<uint8_t>(size))
{
    this->size = buf->size();
    skip(0);
}

buffer::buffer(const std::vector<uint8_t> &buf, uint32_t data_offset)
    : buf(new std::vector<uint8_t>(buf)), data_offset(data_offset)
{
    skip(0);
    size = this->buf->size();
}

buffer::buffer(buffer &rhs, uint32_t size)
    : buf(rhs.buf)
{
    index = rhs.index;
    data_offset = rhs.data_offset;
    ptr = rhs.ptr;
    this->size = index + size;
    rhs.skip(size);
}

buffer::buffer(buffer &rhs, uint32_t size, uint32_t offset)
    : buf(rhs.buf)
{
    index = offset;
    data_offset = offset;
    ptr = (uint8_t *)buf->data() + index;
    this->size = index + size;
}

uint32_t buffer::read(void *dst, uint32_t size, bool nothrow) const
{
    if (!buf)
        throw std::logic_error("buffer: not initialized");
    if (index >= this->size)
    {
        if (nothrow)
            return 0;
        throw std::logic_error("buffer: out of range");
    }
    if (index + size > this->size)
    {
        if (!nothrow)
            throw std::logic_error("buffer: too much data");
        size = this->size - index;
    }
    memcpy(dst, buf->data() + index, size);
    skip(size);
    return size;
}

uint32_t buffer::write(const void *src, uint32_t size, bool nothrow)
{
    if (!buf)
    {
        buf = std::make_shared<std::vector<uint8_t>>(size);
        this->size = buf->size();
    }
    if (index > this->size)
    {
        if (nothrow)
            return 0;
        throw std::logic_error("buffer: out of range");
    }
    if (index + size > this->size)
    {
        buf->resize(index + size);
        this->size = buf->size();
    }
    memcpy((uint8_t *)buf->data() + index, src, size);
    skip(size);
    return size;
}

void buffer::skip(int n) const
{
    if (!buf)
        throw std::logic_error("buffer: not initialized");
    index += n;
    data_offset += n;
    ptr = (uint8_t *)buf->data() + index;
}

void buffer::reset() const
{
    index = 0;
    data_offset = 0;
    ptr = (uint8_t *)buf->data();
}

bool buffer::check(int index) const
{
    return this->index == index;
}

bool buffer::eof() const
{
    return check(this->size);
}

uint32_t buffer::getIndex() const
{
    return index;
}

uint32_t buffer::getSize() const
{
    return this->size;
}

const std::vector<uint8_t> &buffer::getBuf() const
{
    if (!buf)
        throw std::logic_error("buffer: not initialized");
    return *buf;
}
