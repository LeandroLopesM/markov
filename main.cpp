#include <atomic>
#include <thread>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>

using namespace std;

struct token;

struct path
{
    token* p;
    int weight;
};

struct token {
    string content;    
    vector<path> paths; 
};

int sum(const token* a)
{
    int ret = 0; 
    for(path b : a->paths)
    {
        ret += b.weight; 
    }

    return ret;
}

token* random_weighed(token* e, int sums)
{
    int r = (rand() % sums) + 1;

    for(path curr : e->paths )
    {
        if(r <= curr.weight) {
            return curr.p;
        }
        else r -= curr.weight;
    }

    return nullptr;
}

string generate_sentence(vector<token> a, int size)
{
    srand(time(NULL));
    token* next = &a[0];
    string fina = "";

    while(size--)
    {
        int sums = sum(next);
        next = random_weighed(next, sums);
        if(next == nullptr) {
            cout << "SIGSEGV'd in random_weighed" << endl;
            return "";
        }

        fina += next->content + " ";
    }

    return fina;
}

int index_of_str_in_path(string target, const vector<path>& paths)
{
    int i = 0;
    for(path p : paths)
    {
        if(p.p->content == target) return i;
        i++;
    }
    return -1;
}

vector<string> remove_dupes(vector<string> riddled)
{
    vector<string> deduped_vec;

    for(string s : riddled)
    {
        for(auto e : deduped_vec)
            if(s == e) goto skip;

        deduped_vec.push_back(s);
        skip: ;
    }

    return deduped_vec;
}

token* str_in_weights(vector<token>& e, string needle)
{
    for(token& a : e)
    {
        if(a.content == needle) return &a;
    }

    return nullptr;
}

vector<token> tokenize(const vector<string>& s)
{
    vector<token> a;
    vector<string> s_nocopies = remove_dupes(s);
    
    for(string ss : s_nocopies)
    {
        token pre = {
            .content = ss,
            .paths{}
        };
        a.push_back(pre);
    }

    for(token& b : a)
    {
        for(int i = 0; i < s_nocopies.size(); ++i)
        {
            b.paths.push_back(path {
                .p = str_in_weights(a, s_nocopies[i]),
                .weight = 1
            });
        }
    }

    return a;
}

bool weight_compare(const path &a, const path &b)
{
    return a.weight > b.weight;
}

void weigh(vector<token>& a, const vector<string> s)
{
	int index = 0;
    for(token& b : a)
    {
        for(int i = 0; i < s.size(); ++i)
        {
            int c = -1;
            if(i != s.size() - 1 && s[i] == b.content)
			{
                int c = index_of_str_in_path(s[i + 1], b.paths);
                b.paths[c].weight += 2;
				if(i != s.size() - 2)
				{
					c = index_of_str_in_path(s[i + 2], b.paths);
					b.paths[c].weight++;
				}
            }
        }
		
		// order paths
		sort(b.paths.begin(), b.paths.end(), weight_compare);
    }
}

void dump_weights(const vector<token>& e, string streamname)
{
	ofstream stream(streamname);
    for(token a : e)
    {
        stream << "\"" << a.content << "\"" << endl;
        for(auto x : a.paths)
        {
            stream << "\tS \"" << x.p->content << "\"," << endl;
			stream << "\tW " << x.weight << "" << endl << endl;
        }
    }
}

char as_lower(const char c)
{
    if(c <= 'Z' && c >= 'A') return c + 32;
    return c;
}

bool valid_token_char(char c)
{
	// can use ctype::is_alphanum
	// want this here so i have granular control over token characters
	return  (c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9');
}

vector<string> str_to_tok(const string& full_str)
{
    string buff = "";
    vector<string> ret;

	int i = 0; // clean code amirite?

    for( ; i <= full_str.size() ; )
    {
        char c = full_str[i++];

        if(!valid_token_char(c) || i == full_str.size())
		{
            if(buff != "") ret.push_back(buff);
            buff = "";
            continue;
        }

        buff += as_lower(c);
    }

    return ret;
}


void selflearn(vector<token> a, int size, int iterations)
{
    srand(time(NULL));
    token* next = &a[0];
    string fina = "";
    int sizecpy = size;

    while(iterations--)
    {
        size = sizecpy;

        while(size--)
        {
            int sums = sum(next);
            next = random_weighed(next, sums);
            if(next == nullptr) {
                cout << "SIGSEGV'd in random_weighed" << endl;
                return;
            }

            fina += next->content + " ";
        }
        cout << iterations<< " : " << fina << endl;

        auto tok = str_to_tok(fina);        
        // auto e = tokenize(tok);
        weigh(a, tok);
        
        fina = "";
    }
}

atomic<bool> done = false;
void watchdog()
{
	auto time_ran = 0ms;

	while(!done.load())
	{
		if(time_ran >= 5s)
		{
			cerr << "Main thread killed by watchdog (took too long)." << endl;
			exit(1);
		}
		
		time_ran += 250ms;
		this_thread::sleep_for(250ms);
	}
}

string read_file(string path)
{
	std::ifstream in(path.c_str());
	std::ostringstream ss;
	ss << in.rdbuf();

	return ss.str();
}

void memsz(vector<token>& e)
{
	float size = sizeof(token) * e.size() * sizeof(path) * e[0].paths.size();
	string mod = "B";
	if(size / 1024 >= 1) // kb
	{
		mod = "kb";
		size /= 1024;
		if(size / 1024 >= 1) // Mb
		{
			mod = "MB";
			size /= 1024;
			if(size / 1024 >= 1) // GB
			{
				mod = "GB";
				size /= 1024;
			}
		}
	}

	printf("%.1f%s", size, mod.c_str());
}

int main(int argc, char** argv)
{
	if(argc <= 1) return 1;
    std::thread w(watchdog);

    auto tok = str_to_tok(read_file(string(argv[1])));
    
    auto e = tokenize(tok);
    weigh(e, tok);
    cout << generate_sentence(e, 6) << endl; 

	done.store(true);
	dump_weights(e, "Dumps.txt");
	cout << "Total memory taken: "; memsz(e); cout << endl; 
    w.join();
}
