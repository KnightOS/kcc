#include "KCCCleanup.h"

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <string.h>

std::vector<std::string> allowed_commands = {
	".equ",
	".globl",
	".area",
	".org",
	".map",
	".db",
	".ds",
	".dw",
	".ascii",
	".asciiz",
};

std::string trim(const std::string& str,
                 const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::string reduce(const std::string& str,
                   const std::string& fill = " ",
                   const std::string& whitespace = " \t")
{
    // trim first
    auto result = trim(str, whitespace);

    // replace sub ranges
    auto beginSpace = result.find_first_of(whitespace);
    while (beginSpace != std::string::npos)
    {
        const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
        const auto range = endSpace - beginSpace;

        result.replace(beginSpace, range, fill);

        const auto newStart = beginSpace + fill.length();
        beginSpace = result.find_first_of(whitespace, newStart);
    }

    return result;
}

void cleanupFile(const char *file) {
	std::ifstream asm_file(file);
	if (!asm_file) {
		printf("Error: file \"%s\" doesn't exist!\n", file);
		return;
	}
	std::vector<std::string> buffer;
	std::string line, o_line;
	bool dirty = false;
	// Keep the first five lines
	for (int i = 0; i < 4; i++) {
		std::getline(asm_file, o_line);
		buffer.push_back(o_line);
	}
	// Process the rest of them, discarding unneeded pieces
	for (int line_number = 0;std::getline(asm_file, o_line); line_number++) {
		line = reduce(o_line);
		size_t space = line.find(' ');
		bool keep = false;
		if (space != std::string::npos)
			line = line.substr(0, space);
		if (line[0] != ';') {
			if ((line[0] != '.' || line[line.length() - 1] == ':' || std::find(allowed_commands.begin(), allowed_commands.end(), line) != allowed_commands.end())) {
				if (buffer.size() != 0) {
					std::string last_line = buffer.back();
					if (last_line.length() > 6 && last_line.substr(0, 6) == "\t.area"
							&& o_line.length() > 6 && o_line.substr(0, 6) == "\t.area") {
						buffer.pop_back();
					}
				}
				keep = true;
			}
		}
		if (keep) {
			buffer.push_back(o_line);
		}
		else {
			//~ printf("Dropping line: %s\n", o_line.c_str());
			dirty = true;
		}
	}
	asm_file.close();
	if (dirty) {
		std::ofstream output(file);
		if (!output) {
			printf("Error opening file for output!\n");
			return;
		}
		else {
			for (std::string s : buffer) {
				output << s << std::endl;
			}
			output.close();
		}
	}
}

