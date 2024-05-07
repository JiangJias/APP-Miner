#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <bitset>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <unordered_set>
#include <unordered_map>
#include <time.h>
#include <cstdlib>

using namespace std;

const int pathsize = PATHSIZE;
const int locationsize = LOCATIONSIZE;

clock_t startTime;
string file_name;

void isTimeout() {
	double spendTime = double(clock() - startTime) / CLOCKS_PER_SEC;
	if (spendTime > 3600) {
		cout << file_name << " Timeout" << endl;
		exit(0);
	}
}

vector<string> split(const string& s, char delimiter) {
   vector<string> tokens;
   string token;
   istringstream tokenStream(s);
   while (getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
   }
   return tokens;
}

string bit2str(string s) {
	string str;
	bool flag = false;
	int num = 0;
	for (string::reverse_iterator itr = s.rbegin(); itr != s.rend(); itr++) {
		if (*itr == '1') {
			str += to_string(num);
			str += ", ";
			flag = true;
		}
		num++;
	}
	if (flag) {
		str = str.substr(0, str.length() - 2);
	}
	return str;
}

void printAPP(string path, string location, string file) {
	ofstream ofs;
	ofs.open(file, ios::app);
	ofs << "{'path': [";
	ofs << bit2str(path);
	ofs << "], 'location': [";
	ofs << bit2str(location);
	ofs << "]}";
	ofs << endl;
	ofs.close();
}

void frequentMining(vector<bitset<pathsize>> APP, int combination[], unordered_map<string, bitset<locationsize>> &result, string file, int threshold) {
	bitset<pathsize> path;
	path.set();
	for (int j = combination[0]; j > 0; j--) {
		path &= APP[combination[j] - 1];
	}
	if (path.none()) {
		return;
	}
	if (result.find(path.to_string()) == result.end()) {
		bitset<locationsize> location;
		for (int i = 0; i < APP.size(); i++) {
			if ((APP[i] & path) == path) {
				location.set(i);
			}
		}
		vector<string> subpath;
		for (unordered_map<string, bitset<locationsize>>::iterator it = result.begin(); it != result.end(); it++) {
			bitset<pathsize> bit (it->first);
			if ((path & bit) == path) {
				if (location.count() - it->second.count() < threshold) {
					return;
				}
			}
			if ((path & bit) == bit) {
				if (it->second.count() - location.count() < threshold) {
					subpath.push_back(it->first);
				}
			}
		}
		for (vector<string>::iterator it = subpath.begin(); it != subpath.end(); it++) {
			result.erase(*it);
		}
		result.insert(unordered_map<string, bitset<locationsize>>::value_type(path.to_string(), location));
	}
}

int main(int argc, char ** argv) {
	startTime = clock();
	ifstream ifs;
	string APPInputPath, APPOutputPath;
	string line;
	int threshold;
	unordered_map<string, bitset<locationsize>> result;
	//line
	int combination[locationsize * 2];
	vector<bitset<pathsize>> APP(locationsize);
	//row
	unordered_map<string, bitset<locationsize>> APP1, APPlast, APPnew, APPtemp;
	int number;

	if (argc != 4) {
		cout << "args error" << endl;
		return 1;
	}

	file_name = argv[2];

    APPInputPath += "result/";
	APPInputPath += argv[1];
	APPInputPath += "/APPinput/";
	APPInputPath += argv[2];
	APPOutputPath += "result/";
	APPOutputPath += argv[1];
	APPOutputPath += "/APPoutput/";
	APPOutputPath += argv[2];
	threshold = atoi(argv[3]);

	ifs.open(APPInputPath);

	while (getline(ifs, line)) {
		vector<string> lines = split(line, ',');
		bitset<pathsize> path;
		bitset<locationsize> location;
		int pathindex = atoi((*(lines.begin())).c_str());
		path.set(pathindex);
		for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++) {
			if (it == lines.begin()) {
				continue;
			}
			int locationindex = atoi((*it).c_str());
			location.set(locationindex);
		}
		APP1.insert(unordered_map<string, bitset<locationsize>>::value_type(path.to_string(), location));
		APPnew.insert(unordered_map<string, bitset<locationsize>>::value_type(path.to_string(), location));
		result.insert(unordered_map<string, bitset<locationsize>>::value_type(path.to_string(), location));
	}

	number = 1;
	while (1) {
		bool flag = false;
		number += 1;
		APPlast.clear();
		APPtemp = APPlast;
		APPlast = APPnew;
		APPnew = APPtemp;
		for (unordered_map<string, bitset<locationsize>>::iterator it1 = APP1.begin(); it1 != APP1.end(); it1++) {
			for (unordered_map<string, bitset<locationsize>>::iterator it2 = APPlast.begin(); it2 != APPlast.end(); it2++) {
				bitset<pathsize> path;
				bitset<locationsize> location;
				bitset<pathsize> bit1 (it1->first);
				bitset<pathsize> bit2 (it2->first);
				isTimeout();
				if ((bit1 & bit2).any()) {
					continue;
				}
				path = bit1 | bit2;
				if (APPnew.find(path.to_string()) != APPnew.end()) {
					continue;
				}
				location = it1->second & it2->second;
				if (location.count() >= threshold) {
					flag = true;
					APPnew.insert(unordered_map<string, bitset<locationsize>>::value_type(path.to_string(), location));
					result.insert(unordered_map<string, bitset<locationsize>>::value_type(path.to_string(), location));
					for (unordered_map<string, bitset<locationsize>>::iterator it3 = APPlast.begin(); it3 != APPlast.end(); it3++) {
						if ((it3->second.count() - location.count() < threshold) && (result.find(it3->first) != result.end())) {
							result.erase(it3->first);
						}
					}
				}
			}
		}
		if (!flag) {
			break;
		}
	}
	for (unordered_map<string, bitset<locationsize>>::iterator it = result.begin(); it != result.end(); it++) {
		printAPP(it->first, it->second.to_string(), APPOutputPath);
	}
	return 0;
}
