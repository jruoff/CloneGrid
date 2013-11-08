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


#ifndef CLONE_GRID_H
#define CLONE_GRID_H

#include "idrawable.h"
#include <boost/filesystem.hpp>
#include <FTGL/ftgl.h>

class CloneGrid : public virtual IDrawable
{
public:
	CloneGrid(int runs = 4);
	virtual ~CloneGrid();
	
	// Implement IDrawable
	virtual void draw(double scale, int width, int height, int px, int py);
	virtual double size() { return m_size; }
	
	void read_source(const boost::filesystem::path &path);
	void print_statistics();
	void finalize();
	void setup();
	
private:
	struct SourceFile {
		SourceFile(const boost::filesystem::path &path, int position)
			: m_path(path), m_position(position) {}
		
		std::size_t read();
		std::size_t line_count() const { return m_index.size() - 1; }
		std::string::const_iterator line(int i) const { return m_index[i]; }
		
		std::vector<std::string::iterator> m_index;
		std::string m_data;
		boost::filesystem::path m_path;
		int m_position;
	};
	
	friend std::ostream &operator<<(std::ostream &out, const SourceFile &file);
	
	struct SourceLine {
		SourceLine(SourceFile *file, int number)
			: m_file(file), m_number(number) {}
		
		std::string::const_iterator operator[](int i) const
		{ return m_file->line(m_number + i); }
		
		SourceFile *m_file;
		int m_number;
	};
	
	typedef std::vector<SourceFile *> Files;
	typedef std::vector<SourceLine> Lines;
	typedef std::vector<Lines> LineGroups;
	
	Files m_files;
	Lines m_lines;
	LineGroups m_duplicates;
	
	SourceFile *get_file(int position);
	
	int m_runs;
	int m_size   = 0;
	int m_bytes  = 0;
	int m_clones = 0;
	int m_lcount = 0;
	
	unsigned int vboId[2];
	
	void read_lines(const boost::filesystem::path &path);
	void draw_snippet(int left, int top, int pc, double scale);
	
	FTTextureFont m_font;
};

#endif // CLONE_GRID_H
