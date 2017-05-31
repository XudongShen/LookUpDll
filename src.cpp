#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

size_t totalLength;

class Section {
public:
	char name[10];
	size_t length;
	size_t offset;
	size_t flength;
	size_t foffset;
	size_t attr;

	Section() {
		memset(name, 0, 10);
	}

	size_t offsetInFile(size_t off) {
		if (off < offset || off >= offset + length)
			return 0;
		else
			return off - offset + foffset;
	}

};

unsigned char* LoadFile(char* filename) {
	FILE *fp;
	long size;
	unsigned char* buf;
	size_t result;

	if ((fp = fopen(filename, "rb")) == NULL)
		return NULL;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	buf = (unsigned char*)calloc(size, sizeof(char));

	result = fread(buf, 1, size, fp);
	if (result != size)
		return NULL;

	totalLength = result;
	fclose(fp);

	return buf;
}

std::vector<Section> sections;
size_t PEoffset;

void getSections(unsigned char* buf) {
	size_t sectionCount;
	sectionCount = buf[PEoffset + 6] + buf[PEoffset + 7] * 0x100;
	for (int i = 0; i < sectionCount; i++) {
		Section s;
		memcpy(s.name, buf + i * 0x28 + 0xf8 + PEoffset, 8);
		s.length = *(size_t*)(&buf[i * 0x28 + 0xf8 + PEoffset + 8]);
		s.offset = *(size_t*)(&buf[i * 0x28 + 0xf8 + PEoffset + 0xc]);
		s.flength = *(size_t*)(&buf[i * 0x28 + 0xf8 + PEoffset + 0x10]);
		s.foffset = *(size_t*)(&buf[i * 0x28 + 0xf8 + PEoffset + 0x14]);
		s.attr = *(size_t*)(&buf[i * 0x28 + 0xf8 + PEoffset + 0x24]);
		sections.push_back(s);
	}
}

int main(int argc, char* argv[]) {
	unsigned char *buf;	
	char tmp[4] = {};
	size_t ifoffset, icount, nameoffset, t, s;

	if (argc != 2) {
		printf("error input\n");
		return 0;
	}

	buf = LoadFile(argv[1]);
	if (buf == NULL) {
		printf("open file failed\n");
		return 0;
	}
		
	PEoffset = *(size_t*)(&buf[0x3c]);
	memcpy(tmp, buf + PEoffset, 2);
	if (strcmp(tmp, "PE")) {
		printf("it isn't a PE file\n");
		return 0;
	}

	getSections(buf);
	t = *(size_t*)(&buf[PEoffset + 0x80]);
	for (int i = 0; i < sections.size(); i++) {
		if ((ifoffset = sections[i].offsetInFile(t)) != 0) {
			break;
		}
	}

	for(int i=0;;i++) {
		t = *(size_t*)(&buf[ifoffset + 0xc + i*0x14]);
		if (t == 0)
			break;
		for (int j = 0; j < sections.size(); j++) {
			if ((nameoffset = sections[j].offsetInFile(t)) != 0) {
				break;
			}
		}
		printf("%s:\n", buf+nameoffset);
		t = *(size_t*)(&buf[ifoffset + i * 0x14]);
		for (int j = 0; j < sections.size(); j++) {
			if ((s = sections[j].offsetInFile(t)) != 0) {
				break;
			}
		}
		while ((nameoffset = *(size_t*)(&buf[s]))!=0) {
			if (nameoffset & 0x80000000) {
				nameoffset ^= 0x80000000;
				printf("%08X\n", nameoffset);
			}
			else {
				for (int j = 0; j < sections.size(); j++) {
					if ((t = sections[j].offsetInFile(nameoffset)) != 0) {
						break;
					}
				}
				printf("%s\n", buf + t + 2);
			}
			s += 4;
		}
	}

	return 0;
}