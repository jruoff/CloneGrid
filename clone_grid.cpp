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
	
	for (Lines *lines : m_duplicates)
		delete lines;
}

void CloneGrid::read_source(const fs::path &path)
{
	boost::regex exclude(".*/build|.*/\\..*");
	boost::regex include(".*\\.(h|c|hpp|cpp|cc|cs|java|py|rb|php|hs)");
	
	try {
		for (fs::recursive_directory_iterator it(path), end; it != end; ++it)
			if (boost::regex_match(it->path().string(), exclude)) {
				it.no_push();
			} else if (
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
	
	std::cout << "Top-10 Biggest files:\n";
	
	auto compare = [] (SourceFile *a, SourceFile *b) -> bool {
		return a->size() > b->size();
	};
	
	FileSet files(m_files);
	if (files.size() > 10) {
		std::nth_element(files.begin(), files.begin() + 9, files.end(), compare);
		files.erase(files.begin() + 10, files.end());
	}
	std::sort(files.begin(), files.end(), compare);
	
	int tloc = 0;
	for (SourceFile *file : files) {
		tloc += file->size();
		file->print(std::cout);
	}
	
	std::cout << boost::format("LOC: %d (%.3f%%)\n\n") % tloc % (double(tloc)/m_size*100.);
}

void CloneGrid::finalize()
{
	m_lines.clear();
	
	m_duplicates.erase(std::remove_if(
		m_duplicates.begin(), m_duplicates.end(),
		[] (Lines *lines) { return lines->size() > 10; }
	), m_duplicates.end());
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
	
	std::vector<std::string> &lines = file->m_lines;
	for (int i = 0; i <= int(lines.size()) - m_runs; i++) {
		SourceLine line(file, i);
		LineMap::iterator it = m_lines.find(line);
		if (it == m_lines.end()) {
			m_lines[line] = nullptr;
		} else {
			Lines *&value = it->second;
			if (!value) {
				value = new Lines();
				m_duplicates.push_back(value);
				value->push_back(it->first);
			}
			value->push_back(line);
		}
	}
	
	m_size += lines.size();
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

void CloneGrid::draw(double scale)
{
	// TODO: Use a VAO for this static geometry.
	
	glBegin(GL_LINES);
	
	// Draw file borders:
	glColor4f(1, 0, 0, .6 * sqrt(scale));
	for (SourceFile *file : m_files) {
		int p = file->m_position;
		glVertex2i(p, 0); glVertex2i(p, m_size);
		glVertex2i(0, p); glVertex2i(m_size, p);
	}
	glVertex2i(m_size, 0); glVertex2i(m_size, m_size);
	glVertex2i(0, m_size); glVertex2i(m_size, m_size);
	
	// Draw blue diagonal:
	glColor4f(0, 0, 1, 1);
	glVertex2i(0, 0); glVertex2i(m_size, m_size);
	
	glEnd();
	
	// Draw clone dots:
	glBegin(GL_POINTS);
	glColor4f(1, 1, 1, sqrt(scale));
	for (Lines *lineset : m_duplicates) {
		//if (lineset->size() > 10) continue;
		
		for (SourceLine &line1 : *lineset)
		for (SourceLine &line2 : *lineset) {
			int x = line1.m_file->m_position + line1.m_number;
			int y = line2.m_file->m_position + line2.m_number;
			glVertex2i(x, y);
		}
	}
	glEnd();
}
