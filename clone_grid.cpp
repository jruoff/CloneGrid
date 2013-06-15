/*
 * Copyright (c) 2013, Jasper Ruoff <jruoff@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define GL_GLEXT_PROTOTYPES

#include "clone_grid.h"

#include <fstream>
#include <GL/glut.h>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <algorithm>

namespace fs = boost::filesystem;

class Line
{
private:
	std::string data;
public:
	friend std::istream &operator>>(std::istream &is, Line &l) {
		char c;
		l.data.clear();
		while (!is.get(c).eof() && c != '\n')
			l.data += c;
		return is;
	}
	operator std::string() const { return data; }    
};

CloneGrid::~CloneGrid()
{
	for (SourceFile *file : m_files)
		delete file;
}

void CloneGrid::read_source(const fs::path &path)
{
	boost::regex exclude(".*/build|.*/\\..*");
	boost::regex include(".*\\.(h|c|hpp|cpp|cc|cs|java|py|rb|php|hs)");
	
	try {
		for (fs::recursive_directory_iterator it(path), end; it != end; ++it)
			if (boost::regex_match(it->path().string(), exclude))
				it.no_push();
			else if (
				it->symlink_status().type() == fs::regular_file &&
				boost::regex_match(it->path().string(), include)
			)
				read_lines(it->path());
		
	} catch (fs::filesystem_error &e) {
		std::cerr << e.what() << "\n";
	}
}

void CloneGrid::print_statistics()
{
	std::cout << boost::format("Total size:  %.3f MiB\n") % (m_bytes / 1024. / 1024.);
	std::cout << "Total LOC:   " << m_size << "\n";
	std::cout << "Total files: " << m_files.size() << "\n\n";
	
	std::size_t n = std::min(std::size_t(10), m_files.size());
	std::cout << "Top-" << n << " biggest files:\n";
	
	std::partial_sort(
		begin(m_files), begin(m_files) + n, end(m_files),
		[] (SourceFile *a, SourceFile *b) { return a->size() > b->size(); }
	);
	
	int tloc = 0;
	for (std::size_t i = 0; i < n; ++i) {
		tloc += m_files[i]->size();
		m_files[i]->print(std::cout);
	}
	
	std::cout << boost::format("LOC: %d (%.3f%%)\n\n") % tloc % (double(tloc)/m_size*100.);
}

void CloneGrid::finalize()
{
	auto first = begin(m_lines);
	auto last  = end(m_lines);
	
	LineCompare compare(m_runs);
	std::sort(first, last, compare);
	
	auto equal = [&] (const SourceLine &a, const SourceLine &b) {
		return !compare(a, b) && !compare(b, a);
	};
	
	while ((first = std::adjacent_find(first, last, equal)) != last) {
		auto not_equal = [=] (const SourceLine &e) { return !equal(*first, e); };
		auto limit = std::find_if(first, last, not_equal);
		if (limit - first < 10)
			m_duplicates.emplace_back(first, limit);
		first = limit;
	}
}

void CloneGrid::SourceFile::read()
{
	std::ifstream ifs(m_path.string());
	std::istream_iterator<Line> begin(ifs), end;
	m_lines.assign(begin, end);
}

std::ostream &CloneGrid::SourceFile::print(std::ostream &out) const
{
	return out << boost::format("%5d: %s\n") % size() % m_path;
}

void CloneGrid::read_lines(const fs::path &path)
{
	m_bytes += fs::file_size(path);
	
	SourceFile *file = new SourceFile(path, m_size);
	m_files.push_back(file);
	file->read();
	file->print(std::cout);
	
	for (int i = 0; i <= int(file->m_lines.size()) - m_runs; i++)
		m_lines.emplace_back(file, i);
	
	m_size += file->m_lines.size();
}

bool CloneGrid::LineCompare::operator()(const SourceLine &a, const SourceLine &b)
{
	for (int i = 0; i < m_runs; i++) {
		std::string &line1 = a.m_file->m_lines[a.m_number + i];
		std::string &line2 = b.m_file->m_lines[b.m_number + i];
		
		int v = line1.compare(line2);
		if (v != 0) return v < 0;
	}
	
	return false;
}

void CloneGrid::setup()
{
	glGenBuffers(2, vboId);

	// Setup file borders:
	m_lcount = 2 * m_files.size() + 2;
	std::vector<float> vertices;
	vertices.reserve(m_lcount * 4);

	float size = float(m_size);
	for (SourceFile *file : m_files) {
		float p = float(file->m_position);
		vertices.push_back(p);    vertices.push_back(0);
		vertices.push_back(p);    vertices.push_back(size);
		vertices.push_back(0);    vertices.push_back(p);
		vertices.push_back(size); vertices.push_back(p);
	}

	std::vector<float> v {size, 0, size, size, 0, size, size, size};
	vertices.insert(vertices.end(), v.begin(), v.end());

	glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), &vertices[0]);

	// Setup clone dots:
	vertices.clear();

	for (Lines &lineset : m_duplicates)
		for (SourceLine &line1 : lineset)
		for (SourceLine &line2 : lineset) {
			vertices.push_back(line1.m_file->m_position + line1.m_number);
			vertices.push_back(line2.m_file->m_position + line2.m_number);
			m_clones++;
		}

	glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), &vertices[0]);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CloneGrid::draw(double scale)
{
	// Draw file borders:
	glColor4f(1, 0, 0, .6 * sqrt(scale));
	glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, 0);
	glDrawArrays(GL_LINES, 0, m_lcount * 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Draw blue diagonal:
	glBegin(GL_LINES);
	glColor4f(0, 0, 1, 1);
	glVertex2i(0, 0); glVertex2i(m_size, m_size);
	glEnd();

	// Draw clone dots:
	glColor4f(1, 1, 1, sqrt(scale));
	glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, 0);
	glDrawArrays(GL_POINTS, 0, m_clones);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
