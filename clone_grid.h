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
#include <map>

class CloneGrid : public virtual IDrawable
{
public:
	CloneGrid(int runs = 4) : m_lines(LineCompare(runs)), m_size(0), m_bytes(0), m_runs(runs) {}
	virtual ~CloneGrid();
	
	// Implement IDrawable
	virtual void draw(double scale);
	virtual double size() { return m_size; }
	
	void read_source(const boost::filesystem::path &path);
	void print_statistics();
	void finalize();
	
private:
	class SourceFile {
	public:
		SourceFile(const boost::filesystem::path &path, int position)
			: m_path(path), m_position(position) {}
		
		void read();
		std::ostream &print(std::ostream &out) const;
		std::size_t size() const { return m_lines.size(); }
		
		boost::filesystem::path m_path;
		std::vector<std::string> m_lines;
		int m_position;
	};
	
	class SourceLine {
	public:
		SourceLine(SourceFile *file, int number)
			: m_file(file), m_number(number) {}
		
		SourceFile *m_file;
		int m_number;
	};
	
	class LineCompare {
	public:
		LineCompare(int runs) : m_runs(runs) {}
		bool operator()(const SourceLine &a, const SourceLine &b);
		
	private:
		int m_runs;
	};
	
	typedef std::vector<SourceLine> Lines;
	typedef std::vector<Lines *> LineGroups;
	typedef std::vector<SourceFile *> FileSet;
	typedef std::map<SourceLine, Lines *, LineCompare> LineMap;
	
	FileSet m_files;
	LineMap m_lines;
	LineGroups m_duplicates;
	
	int m_size;
	int m_bytes;
	int m_runs;
	
	void read_lines(const boost::filesystem::path &path);
};

#endif // CLONE_GRID_H
