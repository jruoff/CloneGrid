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
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <parallel/algorithm>

namespace fs = boost::filesystem;

CloneGrid::CloneGrid(int runs) :
	m_runs(runs),
	m_font("/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf")
{
	if (m_font.Error())
		std::cerr << "Could not load font.\n";
	
	m_font.FaceSize(15);
}

CloneGrid::~CloneGrid()
{
	for (SourceFile *file : m_files)
		delete file;
}

void CloneGrid::read_source(const fs::path &path)
{
	boost::regex exclude(".*/(build|test|third_party|\\..*)");
	boost::regex include(".*\\.(h|c|hpp|cpp|cc|cs|java|py|rb|php|hs|sh|y|ll|diff)|CMakeLists\\.txt");
	
	try {
		for (fs::recursive_directory_iterator it(path), last; it != last; ++it)
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
	int clones_0 = 0, clones_1 = 0, clones_2 = 0;
	for (Lines &lineset : m_duplicates) {
		clones_0 += 1;
		clones_1 += 1 * lineset.size();
		clones_2 += 1 * lineset.size() * lineset.size();
	}
	
	std::cout << boost::format("Total size:  %.3f MiB\n") % (m_bytes / 1024. / 1024.);
	std::cout << "Total LOC:   " << m_size << "\n";
	std::cout << "Total files: " << m_files.size() << "\n";
	std::cout << "Clones:      " << clones_0 << ":" << clones_1 << ":" << clones_2 << "\n";

	std::size_t n = std::min(std::size_t(10), m_files.size());
	std::cout << "\nTop-" << n << " biggest files:\n";
	
	Files files_sorted(m_files);
	std::partial_sort(
		begin(files_sorted), begin(files_sorted) + n, end(files_sorted),
		[] (SourceFile *a, SourceFile *b) { return a->size() > b->size(); }
	);
	
	int tloc = 0;
	for (std::size_t i = 0; i < n; ++i) {
		tloc += files_sorted[i]->size();
		files_sorted[i]->print(std::cout);
	}
	
	std::cout << boost::format("LOC: %d (%.3f%%)\n\n") % tloc % (double(tloc)/m_size*100.);
}

void CloneGrid::finalize()
{
	auto first = begin(m_lines);
	auto last  = end(m_lines);
	
	__gnu_parallel::sort(first, last, [&] (const SourceLine &a, const SourceLine &b) {
		return std::lexicographical_compare(
			a.line(), a.line(m_runs), b.line(), b.line(m_runs)
		);
	});
	
	auto equal = [&] (const SourceLine &a, const SourceLine &b) {
		return std::equal(a.line(), a.line(m_runs), b.line());
	};
	
	while ((first = std::adjacent_find(first, last, equal)) != last) {
		auto not_equal = [=] (const SourceLine &e) { return !equal(*first, e); };
		auto limit = std::find_if(first, last, not_equal);
		if (limit - first < 10)
			m_duplicates.emplace_back(first, limit);
		first = limit;
	}
	
	m_lines.clear();
}

void CloneGrid::SourceFile::read()
{
	std::ifstream ifs(m_path.string());
	std::istreambuf_iterator<char> first(ifs), last;
	m_data.reserve(fs::file_size(m_path));
	m_data.assign(first, last);
	
	auto it = begin(m_data);
	m_index.push_back(it);
	while ((it = std::find(it, end(m_data), '\n')) != end(m_data))
		m_index.push_back(++it);
	
	m_index.push_back(end(m_data));
}

std::ostream &CloneGrid::SourceFile::print(std::ostream &out) const
{
	return out << boost::format("%5d: %s\n") % size() % m_path;
}

CloneGrid::SourceFile *CloneGrid::get_file(int position)
{
	if (position < 0 || position >= m_size) return nullptr;
	
	return *(std::upper_bound(
		begin(m_files), end(m_files), position,
		[] (int pos, const SourceFile *file) { return pos < file->m_position; }
	) - 1);
}

void CloneGrid::read_lines(const fs::path &path)
{
	m_bytes += fs::file_size(path);
	
	SourceFile *file = new SourceFile(path, m_size);
	m_files.push_back(file);
	file->read();
	file->print(std::cout);
	
	for (int i = 0; i <= int(file->size()) - m_runs; i++)
		m_lines.emplace_back(file, i);
	
	m_size += file->size();
}

void CloneGrid::setup()
{
	glGenBuffers(2, vboId);

	// Setup file borders:
	m_lcount = 2 * m_files.size() + 2;
	std::vector<float> vertices;
	vertices.reserve(m_lcount * 4);

	float size = m_size;
	for (SourceFile *file : m_files) {
		float p = file->m_position;
		vertices.insert(end(vertices), {p, 0, p, size, 0, p, size, p});
	}

	vertices.insert(end(vertices), {size, 0, size, size, 0, size, size, size});

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
	m_duplicates.clear();

	glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), &vertices[0]);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CloneGrid::draw_snippet(int left, int top, int pc, double scale)
{
	int font_size = m_font.FaceSize() - 2, lines = 20, n = lines / 4 + 1;

	SourceFile *file = get_file(pc);

	if (!file) return;
	glColor4f(.5, 1, .5, 1);
	m_font.Render(file->m_path.c_str(), -1, FTPoint(left, top - font_size));

	if (scale <= 1/3.) return;
	double a = sqrt(std::max(.0, 1.5 * scale - .5));
	int p = file->m_position - pc;
	int first = std::max(- lines, p);
	int last  = std::min(  lines, p + int(file->size()) - 1);

	for (int i = first; i <= last; i++) {
		glColor4f(.5, 1, .5, a * std::min((lines + 1. - std::abs(i)) / n, 1.));

		std::string line(file->line(i - p), file->line(i - p + 1));
		if (line.back() == '\n') line.resize(line.size() - 1);
		boost::replace_all(line, "\t", "    ");
		boost::replace_all(line, "\r", "");

		m_font.Render(line.c_str(), -1,
			FTPoint(left, - m_font.LineHeight() * i - font_size / 2)
		);
	}
}

void CloneGrid::draw(double scale, int width, int height, int px, int py)
{
	py = m_size - py;
	glTranslated(0, m_size, 0);
	glScaled(1, -1, 1);
	
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
	
	// Draw code snippets:
	int margin = 20;
	glLoadIdentity();
	draw_snippet(margin - width / 2, height / 2 - margin, py, scale);
	draw_snippet(margin            , height / 2 - margin, px, scale);
}
